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
#include <unordered_map>

#include "maplang/graph/Graph.h"
#include "maplang/ISubgraphContext.h"
#include "concurrentqueue.h"
#include "logging.h"

using namespace std;
using json = nlohmann::json;

namespace maplang {

struct DataGraphItem {
  std::shared_ptr<INode> node;
  std::shared_ptr<IPacketPusher> pathablePacketPusher;  // Set for IPathables only.
  std::shared_ptr<const nlohmann::json>
      lastReceivedParameters; // for parameter propagation when the downstream node(s) get a packet from this node.

  bool operator==(const DataGraphItem& other) const {
    return node == other.node;
  }
};

} // namespace maplang

namespace std {
template<>
struct hash<const maplang::DataGraphItem> {
  std::size_t operator()(const maplang::DataGraphItem& item) const {
    return hash<shared_ptr<maplang::INode>>()(item.node);
  }
};

} // namespace std


namespace maplang {

struct DataGraphEdge {
  shared_ptr<GraphElement<DataGraphItem, DataGraphEdge>> otherGraphElement;
  string channel;
};

using DataGraphElement = GraphElement<DataGraphItem, DataGraphEdge>;

struct PushedPacketInfo {
  Packet packet;
  shared_ptr<DataGraphElement> fromGraphElement;
  shared_ptr<DataGraphElement> manualSendToGraphElement;  // when enqueued from DataGraph::sendPacket().

  string channel;
};

class DataGraphImpl final : public enable_shared_from_this<DataGraphImpl> {
 public:
  Graph<DataGraphItem, DataGraphEdge> mGraph;
  const shared_ptr<uv_loop_t> mUvLoop;
  shared_ptr<ISubgraphContext> mSubgraphContext;
  uv_async_t mPacketReadyAsync;
  moodycamel::ConcurrentQueue<PushedPacketInfo> mPacketQueue;
  unordered_map<const INode*, weak_ptr<DataGraphElement>> mNodeToGraphElementMap;

 public:
  DataGraphImpl(const std::shared_ptr<uv_loop_t>& uvLoop) : mUvLoop(uvLoop) {
    mPacketReadyAsync.data = this;
    int status = uv_async_init(mUvLoop.get(), &mPacketReadyAsync,
                               DataGraphImpl::packetReadyWrapper);
    if (status != 0) {
      throw runtime_error("Failed to initialize async event: " +
          string(strerror(status)));
    }
  }

  static void packetReadyWrapper(uv_async_t* handle);
  void packetReady();
  void sendPacketToNode(
      const shared_ptr<DataGraphElement>& receivingGraphElement,
      const Packet& packet);
  void validateNodeTypesAreCompatible(const std::shared_ptr<INode>& node) const;
  bool hasDataGraphElement(const std::shared_ptr<INode>& node, const std::string& pathableId);
  std::shared_ptr<DataGraphElement> getOrCreateDataGraphElement(const std::shared_ptr<INode>& node, const std::string& pathableId);
};

class SubgraphContext : public ISubgraphContext {
 public:
  SubgraphContext(const shared_ptr<DataGraphImpl> impl)
      : mUvLoop(impl->mUvLoop), mImplWeak(impl) {}

  shared_ptr<uv_loop_t> getUvLoop() const override { return mUvLoop; }

 private:
  const shared_ptr<uv_loop_t> mUvLoop;
  const weak_ptr<DataGraphImpl> mImplWeak;
};

class GraphPacketPusher : public IPacketPusher {
 public:
  GraphPacketPusher(const shared_ptr<DataGraphImpl>& impl,
                    const weak_ptr<DataGraphElement>& graphElement)
      : mImpl(impl), mGraphElement(graphElement) {}

  ~GraphPacketPusher() override = default;

  void pushPacket(const Packet* packet, const string& channel) override {
    const auto graphElement = mGraphElement.lock();
    if (graphElement == nullptr) {
      return;
    }

    PushedPacketInfo info;

    Packet packetWithAccumulatedParameters;
    const auto lastReceivedParameters = graphElement->item.lastReceivedParameters;
    if (lastReceivedParameters != nullptr) {
      packetWithAccumulatedParameters.parameters = *lastReceivedParameters;
    }

    if (!packet->parameters.empty()) {
      if (packetWithAccumulatedParameters.parameters.empty()) {
        packetWithAccumulatedParameters.parameters = packet->parameters;
      } else {
        packetWithAccumulatedParameters.parameters.insert(packet->parameters.begin(), packet->parameters.end());
      }
    }
    packetWithAccumulatedParameters.buffers = packet->buffers;

    info.packet = move(packetWithAccumulatedParameters);
    info.fromGraphElement = graphElement;
    info.channel = channel;

    mImpl->mPacketQueue.enqueue(move(info));
    uv_async_send(&mImpl->mPacketReadyAsync);
  }

 private:
  const shared_ptr<DataGraphImpl> mImpl;
  const weak_ptr<DataGraphElement> mGraphElement;
};

void DataGraphImpl::packetReadyWrapper(uv_async_t* handle) {
  auto impl = (DataGraphImpl*)handle->data;
  impl->packetReady();
}

void DataGraphImpl::packetReady() {
  size_t readyPacketCount = mPacketQueue.size_approx();

  static constexpr size_t kMaxDequeueAtOnce = 100;
  PushedPacketInfo pushedPackets[kMaxDequeueAtOnce];
  size_t processedPacketCount = 0;
  while (processedPacketCount < readyPacketCount) {
    const size_t dequeuedPacketCount =
        mPacketQueue.try_dequeue_bulk(pushedPackets, kMaxDequeueAtOnce);
    processedPacketCount += dequeuedPacketCount;

    for (size_t i = 0; i < dequeuedPacketCount; i++) {
      PushedPacketInfo& packetInfo = pushedPackets[i];

      if (packetInfo.fromGraphElement) {
        bool sentToAny = false;
        for (const auto& nextEdge : packetInfo.fromGraphElement->forwardEdges) {
          const auto& nextEdgeChannelItemPair = nextEdge.first;
          const string& channel = nextEdgeChannelItemPair.channel;
          const shared_ptr<DataGraphElement> nextDataGraphElement = nextEdgeChannelItemPair.toGraphElement;

          if (channel == packetInfo.channel) {
            sentToAny = true;
            sendPacketToNode(nextDataGraphElement, packetInfo.packet);
          }
        }

        if (!sentToAny) {
          logd("Dropped packet from channel %s\n", packetInfo.channel.c_str());
        }
      } else if (packetInfo.manualSendToGraphElement) {
        sendPacketToNode(packetInfo.manualSendToGraphElement, packetInfo.packet);
      }
    }
  }
}

void DataGraphImpl::sendPacketToNode(
    const shared_ptr<DataGraphElement>& receivingGraphElement,
    const Packet& packet) {
  receivingGraphElement->item.lastReceivedParameters = make_shared<json>(packet.parameters);

  const auto pathable = receivingGraphElement->item.node->asPathable();
  if (pathable) {
    PathablePacket pathablePacket;
    pathablePacket.parameters = packet.parameters;
    pathablePacket.buffers = packet.buffers;
    pathablePacket.packetPusher = receivingGraphElement->item.pathablePacketPusher;

    pathable->handlePacket(&pathablePacket);
  } else {
    const auto sink = receivingGraphElement->item.node->asSink();
    sink->handlePacket(&packet);
  }
}

DataGraph::DataGraph(const std::shared_ptr<uv_loop_t>& uvLoop) : impl(new DataGraphImpl(uvLoop)) {
  impl->mSubgraphContext = make_shared<SubgraphContext>(impl);
}

DataGraph::~DataGraph() = default;

void DataGraph::connect(
    const std::shared_ptr<INode>& fromNode,
    const std::string& fromChannel,
    const std::shared_ptr<INode>& toNode,
    const std::string& fromPathableId,
    const std::string& toPathableId) {

  if (fromNode == nullptr) {
    throw runtime_error("fromNode cannot be NULL.");
  } else if (toNode == nullptr) {
    throw runtime_error("toNode cannot be NULL.");
  }

  const bool firstTimeFromNodeWasAdded = !impl->hasDataGraphElement(fromNode, fromPathableId);
  const bool firstTimeToNodeWasAdded = !impl->hasDataGraphElement(toNode, toPathableId);

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
    throw runtime_error("fromPathableId is set, but the Node is not a Pathable.");
  } else if(!toPathable && !toPathableId.empty()) {
    throw runtime_error("toPathableId is set, but the Node is not a Pathable.");
  }

  const auto fromGraphElement = impl->getOrCreateDataGraphElement(fromNode, fromPathableId);
  const auto toGraphElement = impl->getOrCreateDataGraphElement(toNode, toPathableId);

  impl->mGraph.connect(fromGraphElement->item, fromChannel, toGraphElement->item, fromPathableId, toPathableId);
}

void DataGraphImpl::validateNodeTypesAreCompatible(const shared_ptr<INode>& node) const {
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

void DataGraph::sendPacket(const Packet* packet,
                           const std::shared_ptr<INode>& toNode,
                           const string& toPathableId) {
  const auto sink = toNode->asSink();
  if (!sink) {
    throw runtime_error("Can only call sendPacket() with an ISink node.");
  }

  PushedPacketInfo packetInfo;
  packetInfo.packet = *packet;
  packetInfo.manualSendToGraphElement = impl->getOrCreateDataGraphElement(toNode, toPathableId);

  impl->mPacketQueue.enqueue(move(packetInfo));
  uv_async_send(&impl->mPacketReadyAsync);
}


bool DataGraphImpl::hasDataGraphElement(const std::shared_ptr<INode>& node, const std::string& pathableId) {
  DataGraphItem item;
  item.node = node;

  return mGraph.hasItem(item, pathableId);
}

std::shared_ptr<DataGraphElement> DataGraphImpl::getOrCreateDataGraphElement(
    const std::shared_ptr<INode>& node, const std::string& pathableId) {
  const auto it = mNodeToGraphElementMap.find(node.get());
  shared_ptr<DataGraphElement> graphElement;
  if (it == mNodeToGraphElementMap.end()) {
    DataGraphItem item;
    item.node = node;

    validateNodeTypesAreCompatible(node);
    node->setSubgraphContext(mSubgraphContext);

    auto pathable = node->asPathable();
    auto source = node->asSource();
    graphElement = mGraph.getOrCreateGraphElement(item, pathableId);

    if (pathable != nullptr) {
      graphElement->item.pathablePacketPusher = make_shared<GraphPacketPusher>(shared_from_this(), graphElement);
    } else if (source != nullptr) {
      source->setPacketPusher(make_shared<GraphPacketPusher>(shared_from_this(), graphElement));
    }
  }

  return graphElement;
}

}  // namespace maplang
