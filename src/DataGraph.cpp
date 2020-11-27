/*
 * Copyright 2020 VDO Dev Inc <support@maplang.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "maplang/DataGraph.h"

#include <uv.h>

#include <list>
#include <set>
#include <unordered_map>

#include "concurrentqueue.h"
#include "logging.h"
#include "maplang/ISubgraphContext.h"
#include "maplang/graph/Graph.h"

using namespace std;
using json = nlohmann::json;

namespace maplang {

const string DataGraph::kDefaultThreadGroupName = "";

struct DataGraphItem {
  std::shared_ptr<INode> node;
  std::shared_ptr<IPacketPusher>
      pathablePacketPusher;  // Set for IPathables only.
  std::shared_ptr<const nlohmann::json>
      lastReceivedParameters;  // for parameter propagation when the downstream
                               // node(s) get a packet from this node.

  bool operator==(const DataGraphItem& other) const {
    return node == other.node;
  }
};

}  // namespace maplang

namespace std {
template <>
struct hash<const maplang::DataGraphItem> {
  std::size_t operator()(const maplang::DataGraphItem& item) const {
    return hash<shared_ptr<maplang::INode>>()(item.node);
  }
};

}  // namespace std

namespace maplang {

class ThreadGroup;

struct ExtraGraphElementInfo final {
  bool packetPusherWasSetup = false;
  shared_ptr<ThreadGroup> threadGroup;
};

struct DataGraphEdge {
  shared_ptr<GraphElement<DataGraphItem, DataGraphEdge, ExtraGraphElementInfo>>
      otherGraphElement;
  string channel;
  PacketDeliveryType sameThreadQueueToTargetType =
      PacketDeliveryType::PushDirectlyToTarget;
};

using DataGraphElement =
    GraphElement<DataGraphItem, DataGraphEdge, ExtraGraphElementInfo>;

struct PushedPacketInfo {
  Packet packet;
  shared_ptr<DataGraphElement> fromGraphElement;
  shared_ptr<DataGraphElement>
      manualSendToGraphElement;  // when enqueued from DataGraph::sendPacket().
  thread::id queuedFromThreadId;

  string channel;

  // Used to only warn if a packet was not direct- or async-dispatched.
  bool wasDirectDispatchedToAtLeastOneNode = false;
};

struct ThreadGroup {
  ThreadGroup(
      const shared_ptr<UvLoopRunner>& uvLoopRunner,
      const shared_ptr<DataGraphImpl>& dataGraphImpl);

  static void packetReadyWrapper(uv_async_t* handle);
  void packetReady();
  void sendPacketToNode(
      const shared_ptr<DataGraphElement>& receivingGraphElement,
      const Packet& packet);

 public:
  const shared_ptr<UvLoopRunner> mUvLoopRunner;
  const shared_ptr<ISubgraphContext> mSubgraphContext;
  uv_async_t mPacketReadyAsync;
  moodycamel::ConcurrentQueue<PushedPacketInfo> mPacketQueue;
  thread::id mUvLoopThreadId;
};

class DataGraphImpl final : public enable_shared_from_this<DataGraphImpl> {
 public:
  Graph<DataGraphItem, DataGraphEdge, ExtraGraphElementInfo> mGraph;
  const shared_ptr<IUvLoopRunnerFactory> mUvLoopRunnerFactory;
  map<string, shared_ptr<ThreadGroup>> mThreadGroups;

  // node -> pathable ID -> DataGraphElement
  unordered_map<const INode*, unordered_map<string, weak_ptr<DataGraphElement>>>
      mNodeToGraphElementMap;

 public:
  DataGraphImpl(
      const std::shared_ptr<IUvLoopRunnerFactory>& uvLoopRunnerFactory)
      : mUvLoopRunnerFactory(uvLoopRunnerFactory) {}

  shared_ptr<ThreadGroup> getOrCreateThreadGroup(const string& name);

  static void packetReadyWrapper(uv_async_t* handle);
  void validateNodeTypesAreCompatible(const std::shared_ptr<INode>& node) const;
  bool hasDataGraphElement(
      const std::shared_ptr<INode>& node,
      const std::string& pathableId);
  std::shared_ptr<DataGraphElement> getOrCreateDataGraphElement(
      const std::shared_ptr<INode>& node,
      const std::string& pathableId);
  void setupPacketPusher(const shared_ptr<DataGraphElement>& graphElement);

  static void logDroppedPacket(const Packet& packet, const string& channel);
};

class SubgraphContext : public ISubgraphContext {
 public:
  SubgraphContext(
      const shared_ptr<DataGraphImpl> impl,
      const shared_ptr<uv_loop_t>& uvLoop)
      : mUvLoop(uvLoop), mImplWeak(impl) {}

  shared_ptr<uv_loop_t> getUvLoop() const override { return mUvLoop; }

  void removeFromGraph(INode* removeNode) override {
    const auto impl = mImplWeak.lock();
    if (impl == nullptr) {
      return;
    }

    DataGraphItem removeItem;

    /*
     * This shared_ptr doesn't track the pointer's ref count. DataGraphItem has
     * to be used to remove it from the underlying graph.
     */
    removeItem.node = shared_ptr<INode>(removeNode, [](INode* node) {});
    impl->mGraph.removeItem(removeItem);
    removeItem.node.reset();

    auto mapIt = impl->mNodeToGraphElementMap.find(removeNode);
    if (mapIt == impl->mNodeToGraphElementMap.end()) {
      return;
    }

    impl->mNodeToGraphElementMap.erase(mapIt);
  }

 private:
  const shared_ptr<uv_loop_t> mUvLoop;
  const weak_ptr<DataGraphImpl> mImplWeak;
};

class GraphPacketPusher : public IPacketPusher {
 public:
  GraphPacketPusher(
      const shared_ptr<DataGraphImpl>& impl,
      const weak_ptr<DataGraphElement>& graphElement)
      : mImpl(impl), mGraphElement(graphElement) {}

  ~GraphPacketPusher() override = default;

  void pushPacket(const Packet& packet, const string& channel) override {
    Packet copy = packet;
    pushPacket(move(copy), channel);
  }

  void pushPacket(Packet&& packet, const string& fromChannel) override {
    const auto fromGraphElement = mGraphElement.lock();
    if (fromGraphElement == nullptr) {
      return;
    }

    Packet packetWithAccumulatedParameters = move(packet);
    const auto lastReceivedParameters =
        fromGraphElement->item.lastReceivedParameters;
    if (lastReceivedParameters != nullptr) {
      if (packetWithAccumulatedParameters.parameters == nullptr) {
        packetWithAccumulatedParameters.parameters = *lastReceivedParameters;
      } else {
        // insert() does not overwrite values
        packetWithAccumulatedParameters.parameters.insert(
            lastReceivedParameters->begin(),
            lastReceivedParameters->end());
      }
    }

    bool hasAnyTargets = false;
    bool hasDirectTargets = false;
    set<thread::id> queuedToThreadGroupIds;
    const thread::id thisThreadId = this_thread::get_id();
    /*
     * For each edge:
     *   If the channel matches:
     *     If the edge is direct and is in the same thread group, forward the
     * packet If any channels match which are not direct, queue the packet to
     * each matching thread group.
     */
    for (const auto& nextEdge : fromGraphElement->forwardEdges) {
      const auto& nextEdgeChannelItemPair = nextEdge.first;
      const string& channel = nextEdgeChannelItemPair.channel;
      const shared_ptr<DataGraphElement> nextDataGraphElement =
          nextEdgeChannelItemPair.toGraphElement;

      const auto nextNodesThreadGroup =
          nextDataGraphElement->additionalInfo.threadGroup;
      const auto nextNodesThreadId = nextNodesThreadGroup->mUvLoopThreadId;

      const bool nextNodeIsSameThreadGroup = thisThreadId == nextNodesThreadId;
      const bool pushDirectly =
          nextNodeIsSameThreadGroup
          && nextEdge.second.sameThreadQueueToTargetType
                 == PacketDeliveryType::PushDirectlyToTarget;

      if (channel == fromChannel) {
        hasDirectTargets |= pushDirectly;
        hasAnyTargets = true;

        if (pushDirectly) {
          nextNodesThreadGroup->sendPacketToNode(
              nextDataGraphElement,
              packetWithAccumulatedParameters);
        } else if (
            queuedToThreadGroupIds.find(nextNodesThreadId)
            == queuedToThreadGroupIds.end()) {
          PushedPacketInfo info;
          info.packet = packetWithAccumulatedParameters;
          info.fromGraphElement = fromGraphElement;
          info.channel = fromChannel;
          info.wasDirectDispatchedToAtLeastOneNode = hasDirectTargets;
          info.queuedFromThreadId = thisThreadId;

          nextNodesThreadGroup->mPacketQueue.enqueue(move(info));
          uv_async_send(&nextNodesThreadGroup->mPacketReadyAsync);

          queuedToThreadGroupIds.insert(nextNodesThreadId);
        }
      }
    }

    if (!hasAnyTargets) {
      DataGraphImpl::logDroppedPacket(packet, fromChannel);
    }
  }

 private:
  const shared_ptr<DataGraphImpl> mImpl;
  const weak_ptr<DataGraphElement> mGraphElement;
};

shared_ptr<ThreadGroup> DataGraphImpl::getOrCreateThreadGroup(
    const string& threadGroupName) {
  auto existingIt = mThreadGroups.find(threadGroupName);
  if (existingIt != mThreadGroups.end()) {
    return existingIt->second;
  }

  const auto loopRunner = mUvLoopRunnerFactory->createUvLoopRunner();

  const auto threadGroup =
      make_shared<ThreadGroup>(loopRunner, shared_from_this());
  mThreadGroups.insert(make_pair(threadGroupName, threadGroup));

  return threadGroup;
}

ThreadGroup::ThreadGroup(
    const shared_ptr<UvLoopRunner>& uvLoopRunner,
    const shared_ptr<DataGraphImpl>& dataGraphImpl)
    : mUvLoopRunner(uvLoopRunner),
      mSubgraphContext(make_shared<SubgraphContext>(
          dataGraphImpl,
          uvLoopRunner->getLoop())) {
  memset(&mPacketReadyAsync, 0, sizeof(mPacketReadyAsync));
  int status = uv_async_init(
      mUvLoopRunner->getLoop().get(),
      &mPacketReadyAsync,
      ThreadGroup::packetReadyWrapper);

  mPacketReadyAsync.data = this;

  if (status != 0) {
    throw runtime_error(
        "Failed to initialize async event: " + string(strerror(status)));
  }

  mUvLoopThreadId = mUvLoopRunner->getUvLoopThreadId();
}

void ThreadGroup::packetReadyWrapper(uv_async_t* handle) {
  auto impl = (ThreadGroup*)handle->data;
  impl->packetReady();
}

void ThreadGroup::packetReady() {
  size_t readyPacketCount = mPacketQueue.size_approx();

  static constexpr size_t kMaxDequeueAtOnce = 100;
  PushedPacketInfo pushedPackets[kMaxDequeueAtOnce];
  size_t processedPacketCount = 0;
  const thread::id thisThreadId = this_thread::get_id();

  while (processedPacketCount < readyPacketCount) {
    const size_t dequeuedPacketCount =
        mPacketQueue.try_dequeue_bulk(pushedPackets, kMaxDequeueAtOnce);
    processedPacketCount += dequeuedPacketCount;

    for (size_t i = 0; i < dequeuedPacketCount; i++) {
      PushedPacketInfo& packetInfo = pushedPackets[i];

      if (packetInfo.fromGraphElement) {
        bool sentToAny = false;
        for (const auto& nextEdge : packetInfo.fromGraphElement->forwardEdges) {
          const bool edgeUsesThisThread =
              thisThreadId
              == nextEdge.second.otherGraphElement->additionalInfo.threadGroup
                     ->mUvLoopThreadId;

          if (!edgeUsesThisThread) {
            continue;
          }

          const auto& nextEdgeChannelItemPair = nextEdge.first;
          const string& channel = nextEdgeChannelItemPair.channel;
          const shared_ptr<DataGraphElement> nextDataGraphElement =
              nextEdgeChannelItemPair.toGraphElement;

          const bool queuedFromThisThread =
              thisThreadId == packetInfo.queuedFromThreadId;
          const bool thisEdgeUsesQueuedPackets =
              !queuedFromThisThread
              || nextEdge.second.sameThreadQueueToTargetType
                     == PacketDeliveryType::AlwaysQueue;

          sentToAny |= packetInfo.wasDirectDispatchedToAtLeastOneNode;
          if (thisEdgeUsesQueuedPackets && channel == packetInfo.channel) {
            sentToAny = true;
            sendPacketToNode(nextDataGraphElement, packetInfo.packet);
          }
        }

        if (!sentToAny) {
          DataGraphImpl::logDroppedPacket(
              packetInfo.packet,
              packetInfo.channel);
        }
      } else if (packetInfo.manualSendToGraphElement) {
        sendPacketToNode(
            packetInfo.manualSendToGraphElement,
            packetInfo.packet);
      }
    }
  }
}

void ThreadGroup::sendPacketToNode(
    const shared_ptr<DataGraphElement>& receivingGraphElement,
    const Packet& packet) {
  receivingGraphElement->item.lastReceivedParameters =
      make_shared<json>(packet.parameters);

  const auto pathable = receivingGraphElement->item.node->asPathable();
  if (pathable) {
    PathablePacket pathablePacket(
        packet,
        receivingGraphElement->item.pathablePacketPusher);

    pathable->handlePacket(pathablePacket);
  } else {
    const auto sink = receivingGraphElement->item.node->asSink();
    sink->handlePacket(packet);
  }
}

DataGraph::DataGraph(
    const std::shared_ptr<IUvLoopRunnerFactory>& loopRunnerFactory)
    : impl(new DataGraphImpl(loopRunnerFactory)) {}

DataGraph::~DataGraph() = default;

void DataGraph::connect(
    const std::shared_ptr<INode>& fromNode,
    const std::string& fromChannel,
    const std::shared_ptr<INode>& toNode,
    const std::string& fromPathableId,
    const std::string& toPathableId,
    const PacketDeliveryType& sameThreadQueueToTargetType) {
  if (fromNode == nullptr) {
    throw runtime_error("fromNode cannot be NULL.");
  } else if (toNode == nullptr) {
    throw runtime_error("toNode cannot be NULL.");
  }

  const auto fromPathable = fromNode->asPathable();
  const auto source = fromNode->asSource();
  const auto toPathable = toNode->asPathable();
  const auto sink = toNode->asSink();

  if (!source && !fromPathable) {
    throw runtime_error(
        "Cannot make a graph connection starting from a Node which is not a "
        "source or a pathable.");
  }

  if (!sink && !toPathable) {
    throw runtime_error(
        "Cannot make a graph connection ending with a Node which is not a sink "
        "or a pathable.");
  }

  if (!fromPathable && !fromPathableId.empty()) {
    throw runtime_error(
        "fromPathableId is set, but the Node is not a Pathable.");
  } else if (!toPathable && !toPathableId.empty()) {
    throw runtime_error("toPathableId is set, but the Node is not a Pathable.");
  }

  const auto fromGraphElement =
      impl->getOrCreateDataGraphElement(fromNode, fromPathableId);
  const auto toGraphElement =
      impl->getOrCreateDataGraphElement(toNode, toPathableId);

  auto& edge = impl->mGraph.connect(
      fromGraphElement->item,
      fromChannel,
      toGraphElement->item,
      fromPathableId,
      toPathableId);

  // Setup 'to' first in case from sends a packet in setupPacketPusher().
  impl->setupPacketPusher(toGraphElement);
  impl->setupPacketPusher(fromGraphElement);

  edge.sameThreadQueueToTargetType = sameThreadQueueToTargetType;
}

void DataGraphImpl::validateNodeTypesAreCompatible(
    const shared_ptr<INode>& node) const {
  const auto pathable = node->asPathable();
  const auto sink = node->asSink();
  const auto source = node->asSource();

  const bool isSink = sink != nullptr;
  const bool isSource = source != nullptr;
  const bool isPathable = pathable != nullptr;

  if (isPathable && (isSource || isSink)) {
    throw runtime_error("A pathable cannot be a source or a sink.");
  } else if (!isPathable && !isSource && !isSink) {
    throw runtime_error("A node must be a pathable, sink or source.");
  }
}

void DataGraph::sendPacket(
    const Packet& packet,
    const std::shared_ptr<INode>& toNode,
    const string& toPathableId) {
  const auto sink = toNode->asSink();
  if (!sink) {
    throw runtime_error("Can only call sendPacket() with an ISink node.");
  }

  PushedPacketInfo packetInfo;
  packetInfo.packet = packet;
  packetInfo.manualSendToGraphElement =
      impl->getOrCreateDataGraphElement(toNode, toPathableId);
  packetInfo.queuedFromThreadId = this_thread::get_id();

  impl->setupPacketPusher(packetInfo.manualSendToGraphElement);

  const auto sendToThreadGroup =
      packetInfo.manualSendToGraphElement->additionalInfo.threadGroup;

  sendToThreadGroup->mPacketQueue.enqueue(move(packetInfo));
  uv_async_send(&sendToThreadGroup->mPacketReadyAsync);
}

void DataGraph::sendPacket(
    Packet&& packet,
    const std::shared_ptr<INode>& toNode,
    const string& toPathableId) {
  const auto sink = toNode->asSink();
  if (!sink) {
    throw runtime_error("Can only call sendPacket() with an ISink node.");
  }

  PushedPacketInfo packetInfo;
  packetInfo.packet = move(packet);
  packetInfo.manualSendToGraphElement =
      impl->getOrCreateDataGraphElement(toNode, toPathableId);
  packetInfo.queuedFromThreadId = this_thread::get_id();

  impl->setupPacketPusher(packetInfo.manualSendToGraphElement);

  const auto sendToThreadGroup =
      packetInfo.manualSendToGraphElement->additionalInfo.threadGroup;

  sendToThreadGroup->mPacketQueue.enqueue(move(packetInfo));
  uv_async_send(&sendToThreadGroup->mPacketReadyAsync);
}

bool DataGraphImpl::hasDataGraphElement(
    const std::shared_ptr<INode>& node,
    const std::string& pathableId) {
  DataGraphItem item;
  item.node = node;

  return mGraph.hasItem(item, pathableId);
}

std::shared_ptr<DataGraphElement> DataGraphImpl::getOrCreateDataGraphElement(
    const std::shared_ptr<INode>& node,
    const std::string& pathableId) {
  auto mapIt = mNodeToGraphElementMap.find(node.get());
  shared_ptr<DataGraphElement> graphElement;

  if (mapIt == mNodeToGraphElementMap.end()) {
    auto insertPair = mNodeToGraphElementMap.emplace(make_pair(
        node.get(),
        unordered_map<string, weak_ptr<DataGraphElement>>()));

    mapIt = insertPair.first;
  }

  auto& pathableIdToElementMap = mapIt->second;
  const auto elementIt = pathableIdToElementMap.find(pathableId);
  if (elementIt != pathableIdToElementMap.end()) {
    graphElement = elementIt->second.lock();
  }

  if (graphElement == nullptr) {
    DataGraphItem item;
    item.node = node;

    validateNodeTypesAreCompatible(node);

    graphElement = mGraph.getOrCreateGraphElement(item, pathableId);

    const auto threadGroup =
        getOrCreateThreadGroup(DataGraph::kDefaultThreadGroupName);
    graphElement->additionalInfo.threadGroup = threadGroup;

    node->setSubgraphContext(threadGroup->mSubgraphContext);
    pathableIdToElementMap.insert(make_pair(pathableId, graphElement));
  }

  return graphElement;
}

void DataGraphImpl::logDroppedPacket(
    const Packet& packet,
    const string& channel) {
  if (channel == "error") {
    logi("Dropped error packet: %s\n", packet.parameters.dump(2).c_str());
  } else {
    logd("Dropped packet from channel %s\n", channel.c_str());
  }
}

void DataGraphImpl::setupPacketPusher(
    const shared_ptr<DataGraphElement>& graphElement) {
  const auto node = graphElement->item.node;
  auto pathable = node->asPathable();
  auto source = node->asSource();

  if (!graphElement->additionalInfo.packetPusherWasSetup) {
    graphElement->additionalInfo.packetPusherWasSetup = true;

    if (pathable != nullptr) {
      graphElement->item.pathablePacketPusher =
          make_shared<GraphPacketPusher>(shared_from_this(), graphElement);
    } else if (source != nullptr) {
      source->setPacketPusher(
          make_shared<GraphPacketPusher>(shared_from_this(), graphElement));
    }
  }
}

void DataGraph::setThreadGroupForNode(
    const std::shared_ptr<INode>& toNode,
    const std::string& threadGroupName,
    const std::string& pathableId) {
  const auto graphElement =
      impl->getOrCreateDataGraphElement(toNode, pathableId);

  const auto threadGroup = impl->getOrCreateThreadGroup(threadGroupName);
  graphElement->additionalInfo.threadGroup = threadGroup;
}

}  // namespace maplang
