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

#include <iomanip>
#include <list>
#include <optional>
#include <set>
#include <sstream>
#include <unordered_map>

#include "logging.h"
#include "maplang/ISubgraphContext.h"
#include "maplang/Instance.h"
#include "maplang/Util.h"
#include "maplang/UvLoopRunnerFactory.h"
#include "maplang/concurrentqueue.h"

using namespace std;
using json = nlohmann::json;

namespace maplang {

const string DataGraph::kDefaultThreadGroupName = "";

}  // namespace maplang

namespace maplang {

struct PushedPacketInfo {
  Packet packet;
  shared_ptr<GraphElement> fromElement;
  thread::id queuedFromThreadId;
  string channel;

  // Set when enqueued from DataGraph::sendPacket().
  shared_ptr<GraphElement> manualSendToElement;

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
      const shared_ptr<GraphElement>& receivingElement,
      const Packet& packet);

 public:
  const shared_ptr<UvLoopRunner> mUvLoopRunner;
  const shared_ptr<ISubgraphContext> mSubgraphContext;
  uv_async_t mPacketReadyAsync;
  moodycamel::ConcurrentQueue<PushedPacketInfo> mPacketQueue;
  thread::id mUvLoopThreadId;
  weak_ptr<DataGraphImpl> mDataGraphImpl;
};

class DataGraphImpl final : public enable_shared_from_this<DataGraphImpl> {
 public:
  Graph mGraph;
  const shared_ptr<IUvLoopRunnerFactory> mUvLoopRunnerFactory;
  unordered_map<string, shared_ptr<ThreadGroup>> mThreadGroups;
  unordered_map<string, shared_ptr<Instance>> mInstances;
  shared_ptr<NodeFactory> mNodeFactory;

 public:
  static void packetReadyWrapper(uv_async_t* handle);
  static void logDroppedPacket(const Packet& packet, const string& channel);

  DataGraphImpl(const shared_ptr<IUvLoopRunnerFactory>& uvLoopRunnerFactory)
      : mUvLoopRunnerFactory(uvLoopRunnerFactory),
        mNodeFactory(NodeFactory::defaultFactory()) {}

  shared_ptr<ThreadGroup> getOrCreateThreadGroup(const string& name);
  shared_ptr<Instance> getOrCreateInstance(const string& instanceName);
  shared_ptr<Instance> getInstanceForGraphElement(
      const std::shared_ptr<GraphElement>& graphElement) const;
  void validateConnections(const shared_ptr<GraphElement>& graphElement) const;
  shared_ptr<INode> instantiateNode(
      const string& implementation,
      const json& initParameters);
  shared_ptr<GraphElement> getOrCreateGraphElement(const string& nodeName);
  void setThreadGroupForInstance(
      const string& instanceName,
      const string& threadGroupName);
};

class SubgraphContext : public ISubgraphContext {
 public:
  SubgraphContext(
      const shared_ptr<DataGraphImpl> impl,
      const shared_ptr<uv_loop_t>& uvLoop)
      : mUvLoop(uvLoop), mImplWeak(impl) {}

  shared_ptr<uv_loop_t> getUvLoop() const override { return mUvLoop; }

 private:
  const shared_ptr<uv_loop_t> mUvLoop;
  const weak_ptr<DataGraphImpl> mImplWeak;
};

class GraphPacketPusher : public IPacketPusher {
 public:
  GraphPacketPusher(
      const shared_ptr<DataGraphImpl>& impl,
      const weak_ptr<GraphElement>& graphElement)
      : mImpl(impl), mGraphElement(graphElement) {}

  ~GraphPacketPusher() override = default;

  void pushPacket(const Packet& packet, const string& channel) override {
    Packet copy = packet;
    pushPacket(move(copy), channel);
  }

  void pushPacket(Packet&& packet, const string& fromChannel) override {
    const auto fromElement = mGraphElement.lock();
    if (fromElement == nullptr) {
      return;
    }

    Packet packetWithAccumulatedParameters = move(packet);
    const auto lastReceivedParameters = fromElement->lastReceivedParameters;

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

    vector<pair<shared_ptr<ThreadGroup>, Packet>> asyncThreadGroupPacketPairs;
    for (const auto& nextChannelEdges : fromElement->forwardEdges) {
      const auto& channel = nextChannelEdges.first;

      if (channel != fromChannel) {
        continue;
      }

      const vector<GraphEdge>& channelEdges = nextChannelEdges.second;

      for (const GraphEdge& graphEdge : channelEdges) {
        const shared_ptr<GraphElement> nextElement = graphEdge.next;
        const shared_ptr<Instance> nextInstance =
            mImpl->getInstanceForGraphElement(nextElement);
        const auto nextNodesThreadGroup =
            mImpl->getOrCreateThreadGroup(nextInstance->getThreadGroupName());
        const auto nextNodesThreadId = nextNodesThreadGroup->mUvLoopThreadId;
        const bool nextNodeIsSameThreadGroup =
            thisThreadId == nextNodesThreadId;
        const bool pushDirectly =
            nextNodeIsSameThreadGroup
            && graphEdge.sameThreadQueueToTargetType
                   == PacketDeliveryType::PushDirectlyToTarget;

        hasDirectTargets |= pushDirectly;
        hasAnyTargets = true;

        if (pushDirectly) {
          nextNodesThreadGroup->sendPacketToNode(
              nextElement,
              packetWithAccumulatedParameters);
        } else if (
            queuedToThreadGroupIds.find(nextNodesThreadId)
            == queuedToThreadGroupIds.end()) {
          // Only dispatch once per thread group because the ThreadGroup will
          // send across all connections that should be run on it.
          asyncThreadGroupPacketPairs.emplace_back(
              nextNodesThreadGroup,
              packetWithAccumulatedParameters);

          queuedToThreadGroupIds.insert(nextNodesThreadId);
        }
      }
    }

    for (const auto& threadGroupPacketPair : asyncThreadGroupPacketPairs) {
      const auto& threadGroup = threadGroupPacketPair.first;
      const auto& packetWithAccumulatedParameters =
          threadGroupPacketPair.second;

      PushedPacketInfo info;
      info.packet = packetWithAccumulatedParameters;
      info.fromElement = fromElement;
      info.channel = fromChannel;
      info.wasDirectDispatchedToAtLeastOneNode = hasDirectTargets;
      info.queuedFromThreadId = thisThreadId;

      threadGroup->mPacketQueue.enqueue(move(info));
      uv_async_send(&threadGroup->mPacketReadyAsync);
    }

    if (!hasAnyTargets) {
      DataGraphImpl::logDroppedPacket(packet, fromChannel);
    }
  }

 private:
  const shared_ptr<DataGraphImpl> mImpl;
  const weak_ptr<GraphElement> mGraphElement;
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

shared_ptr<INode> DataGraphImpl::instantiateNode(
    const string& implementation,
    const json& initParameters) {
  return mNodeFactory->createNode(implementation, initParameters);
}

ThreadGroup::ThreadGroup(
    const shared_ptr<UvLoopRunner>& uvLoopRunner,
    const shared_ptr<DataGraphImpl>& dataGraphImpl)
    : mUvLoopRunner(uvLoopRunner),
      mSubgraphContext(
          make_shared<SubgraphContext>(dataGraphImpl, uvLoopRunner->getLoop())),
      mDataGraphImpl(dataGraphImpl) {
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
  const auto impl = mDataGraphImpl.lock();
  if (impl == nullptr) {
    return;
  }

  static constexpr size_t kMaxDequeueAtOnce = 100;
  PushedPacketInfo pushedPackets[kMaxDequeueAtOnce];
  size_t processedPacketCount = 0;
  const thread::id thisThreadId = this_thread::get_id();

  while (true) {
    const size_t dequeuedPacketCount =
        mPacketQueue.try_dequeue_bulk(pushedPackets, kMaxDequeueAtOnce);

    if (dequeuedPacketCount == 0) {
      break;
    }

    processedPacketCount += dequeuedPacketCount;

    for (size_t i = 0; i < dequeuedPacketCount; i++) {
      PushedPacketInfo& packetInfo = pushedPackets[i];

      if (packetInfo.fromElement) {
        bool sentToAny = false;
        for (const auto& nextChannelInfo :
             packetInfo.fromElement->forwardEdges) {
          const string& channel = nextChannelInfo.first;
          const vector<GraphEdge>& channelEdges = nextChannelInfo.second;

          for (const GraphEdge& nextEdge : channelEdges) {
            const auto nextElement = nextEdge.next;
            const auto nextInstance =
                impl->getInstanceForGraphElement(nextElement);
            const auto sendOnThreadGroup = impl->getOrCreateThreadGroup(
                nextInstance->getThreadGroupName());
            const bool edgeUsesThisThread =
                thisThreadId == sendOnThreadGroup->mUvLoopThreadId;

            if (!edgeUsesThisThread) {
              continue;
            }

            const bool queuedFromThisThread =
                thisThreadId == packetInfo.queuedFromThreadId;
            const bool thisEdgeUsesQueuedPackets =
                !queuedFromThisThread
                || nextEdge.sameThreadQueueToTargetType
                       == PacketDeliveryType::AlwaysQueue;

            sentToAny |= packetInfo.wasDirectDispatchedToAtLeastOneNode;
            if (thisEdgeUsesQueuedPackets && channel == packetInfo.channel) {
              sentToAny = true;
              sendPacketToNode(nextElement, packetInfo.packet);
            }
          }
        }

        if (!sentToAny) {
          DataGraphImpl::logDroppedPacket(
              packetInfo.packet,
              packetInfo.channel);
        }
      } else if (packetInfo.manualSendToElement) {
        sendPacketToNode(packetInfo.manualSendToElement, packetInfo.packet);
      }
    }
  }
}

void ThreadGroup::sendPacketToNode(
    const shared_ptr<GraphElement>& receivingElement,
    const Packet& packet) {
  const auto dataGraphImpl = mDataGraphImpl.lock();
  if (dataGraphImpl == nullptr) {
    logw("Dropping packet because DataGraph is gone.");
    return;
  }

  receivingElement->lastReceivedParameters =
      make_shared<json>(packet.parameters);

  const auto receivingInstance =
      dataGraphImpl->getInstanceForGraphElement(receivingElement);

  if (receivingInstance == nullptr) {
    throw runtime_error(
        "An Instance has not been set for Node '"
        + receivingElement->elementName + "'.");
  }

  const auto receivingImplementation = receivingInstance->getImplementation();

  if (receivingImplementation == nullptr) {
    throw runtime_error(
        "Type or Implementation has not been set for Instance '"
        + receivingElement->instanceName + "'.");
  }

  const auto pathable = receivingImplementation->asPathable();
  if (pathable) {
    PathablePacket pathablePacket(packet, receivingElement->packetPusher);

    pathable->handlePacket(pathablePacket);
  } else {
    const auto sink = receivingImplementation->asSink();
    sink->handlePacket(packet);
  }
}

DataGraph::DataGraph()
    : impl(new DataGraphImpl(make_shared<UvLoopRunnerFactory>())) {}

DataGraph::~DataGraph() = default;

void DataGraph::connect(
    const string& fromElementName,
    const string& fromChannel,
    const string& toElementName,
    const PacketDeliveryType& sameThreadQueueToTargetType) {
  if (fromElementName.empty()) {
    throw runtime_error("fromElementName must be set.");
  } else if (toElementName.empty()) {
    throw runtime_error("toElementName must be set.");
  }

  const auto fromElement = impl->getOrCreateGraphElement(fromElementName);
  const auto toElement = impl->getOrCreateGraphElement(toElementName);

  auto& edge =
      impl->mGraph.connect(fromElementName, fromChannel, toElementName);

  edge.sameThreadQueueToTargetType = sameThreadQueueToTargetType;
}

void DataGraph::disconnect(
    const string& fromElementName,
    const string& fromChannel,
    const string& toElementName) {
  impl->mGraph.disconnect(fromElementName, fromChannel, toElementName);
}

void DataGraph::sendPacket(const Packet& packet, const string& toElementName) {
  const shared_ptr<GraphElement> toElement =
      impl->getOrCreateGraphElement(toElementName);

  PushedPacketInfo packetInfo;
  packetInfo.packet = packet;
  packetInfo.manualSendToElement = toElement;
  packetInfo.queuedFromThreadId = this_thread::get_id();

  const auto instance = impl->getInstanceForGraphElement(toElement);
  const auto sendToThreadGroup =
      impl->getOrCreateThreadGroup(instance->getThreadGroupName());

  sendToThreadGroup->mPacketQueue.enqueue(move(packetInfo));
  uv_async_send(&sendToThreadGroup->mPacketReadyAsync);
}

shared_ptr<GraphElement> DataGraphImpl::getOrCreateGraphElement(
    const string& elementName) {
  const bool graphElementIsNewlyCreated = !mGraph.getGraphElement(elementName);

  auto existingElement = mGraph.getGraphElement(elementName);

  if (existingElement) {
    return *existingElement;
  }

  const auto element = mGraph.getOrCreateGraphElement(elementName);

  if (graphElementIsNewlyCreated) {
    element->packetPusher =
        make_shared<GraphPacketPusher>(shared_from_this(), element);
  }

  return element;
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

void DataGraph::setThreadGroupForInstance(
    const string& instanceName,
    const string& threadGroupName) {
  impl->setThreadGroupForInstance(instanceName, threadGroupName);
}

void DataGraphImpl::setThreadGroupForInstance(
    const string& instanceName,
    const string& threadGroupName) {
  const shared_ptr<Instance> instance = getOrCreateInstance(instanceName);
  const auto threadGroup = getOrCreateThreadGroup(threadGroupName);
  instance->setThreadGroupName(threadGroupName);
  instance->setSubgraphContext(threadGroup->mSubgraphContext);
}

void to_json(nlohmann::json& j, const PacketDeliveryType& packetDelivery) {
  switch (packetDelivery) {
    case PacketDeliveryType::PushDirectlyToTarget:
      j = "Push Directly To Target";
      break;

    case PacketDeliveryType::AlwaysQueue:
      j = "Always Queue";
      break;

    default:
      throw invalid_argument(
          "Unknown packet delivery type: "
          + to_string(static_cast<uint32_t>(packetDelivery)));
  }
}

void from_json(const nlohmann::json& j, PacketDeliveryType& packetDelivery) {
  const string str = j.get<string>();

  if (str == "Push Directly To Target") {
    packetDelivery = PacketDeliveryType::PushDirectlyToTarget;
  } else if (str == "Always Queue") {
    packetDelivery = PacketDeliveryType::AlwaysQueue;
  } else {
    throw invalid_argument("Unknown PacketDeliveryType '" + str + "'.");
  }
}

void DataGraph::setNodeInstance(
    const std::string& nodeName,
    const std::string& instanceName) {
  const shared_ptr<GraphElement> graphElement =
      impl->getOrCreateGraphElement(nodeName);

  graphElement->instanceName = instanceName;
  const auto instance = impl->getOrCreateInstance(instanceName);
  instance->setPacketPusherForISources(graphElement->packetPusher);
}

void DataGraph::setInstanceInitParameters(
    const std::string& instanceName,
    const nlohmann::json& initParameters) {
  if (!initParameters.is_object()) {
    throw runtime_error("initParameters must be an object.");
  }

  const shared_ptr<Instance> instance = impl->getOrCreateInstance(instanceName);
  instance->setInitParameters(initParameters);
}

void DataGraph::setInstanceType(
    const std::string& instanceName,
    const std::string& typeName) {
  const shared_ptr<Instance> instance = impl->getOrCreateInstance(instanceName);

  try {
    instance->setType(typeName, impl->mNodeFactory);
  } catch (exception& e) {
    ostringstream errorStream;
    errorStream << "Error instantiating Instance '" << instanceName
                << "' type '" << typeName << "' initParameters " << setw(2)
                << instance->getInitParameters() << ": " << e.what();

    const string errorString = errorStream.str();
    loge("%s\n", errorString.c_str());

    throw e;
  }

  impl->mInstances[instanceName] = instance;
}

void DataGraph::setInstanceImplementation(
    const std::string& instanceName,
    const std::shared_ptr<INode>& implementation) {
  const shared_ptr<Instance> instance = impl->getOrCreateInstance(instanceName);
  instance->setImplementation(implementation);
}

void DataGraph::setInstanceImplementationToGroupInterface(
    const std::string& instanceName,
    const std::string& groupInstanceName,
    const std::string& groupInterfaceName) {
  const auto groupInstanceIt = impl->mInstances.find(groupInstanceName);

  if (groupInstanceIt == impl->mInstances.end()) {
    throw runtime_error(
        "Group Instance Name '" + groupInstanceName
        + "' does not exist. Error while attempting to set Instance '"
        + instanceName + "' implementation to the group's '"
        + groupInterfaceName + "' interface.");
  }

  shared_ptr<Instance> groupInstance = groupInstanceIt->second;
  ICohesiveGroup* group = groupInstance->getImplementation()->asGroup();
  if (group == nullptr) {
    throw runtime_error(
        "Implementation of '" + groupInstanceName
        + "' is not a group. Error while attempting to set Instance '"
        + instanceName + "' implementation to the group's '"
        + groupInterfaceName + "' interface.");
  }

  shared_ptr<INode> groupInterfaceNode = group->getNode(groupInterfaceName);
  if (groupInterfaceNode == nullptr) {
    throw runtime_error(
        "Group implementation for '" + groupInstanceName
        + "' does not contain interface '" + groupInterfaceName
        + "'. Error while attempting to set Instance '" + instanceName + ".");
  }

  const shared_ptr<Instance> instance = impl->getOrCreateInstance(instanceName);
  instance->setImplementation(groupInterfaceNode);
}

shared_ptr<Instance> DataGraphImpl::getOrCreateInstance(
    const string& instanceName) {
  auto instanceIt = mInstances.find(instanceName);

  if (instanceIt != mInstances.end()) {
    return instanceIt->second;
  }

  const auto instance = make_shared<Instance>();
  mInstances[instanceName] = instance;
  setThreadGroupForInstance(instanceName, DataGraph::kDefaultThreadGroupName);

  return instance;
}

shared_ptr<Instance> DataGraphImpl::getInstanceForGraphElement(
    const std::shared_ptr<GraphElement>& graphElement) const {
  const string instanceName = graphElement->instanceName;
  if (instanceName.empty()) {
    throw runtime_error(
        "No instance assigned to GraphElement '" + graphElement->elementName
        + "'.");
  }

  const auto instanceIt = mInstances.find(instanceName);
  if (instanceIt == mInstances.end()) {
    throw runtime_error(
        "Instance '" + instanceName
        + "' does not exist. Needed by GraphElement '"
        + graphElement->elementName + "'.");
  }

  return instanceIt->second;
}

void DataGraph::validateConnections() const {
  visitGraphElements([this](const shared_ptr<GraphElement>& graphElement) {
    impl->validateConnections(graphElement);
  });
}

void DataGraphImpl::validateConnections(
    const shared_ptr<GraphElement>& graphElement) const {
  const auto instance = getInstanceForGraphElement(graphElement);
  const auto implementation = instance->getImplementation();
  if (implementation == nullptr) {
    return;
  }

  graphElement->cleanUpEmptyEdges();

  if (!graphElement->forwardEdges.empty()
      && implementation->asSource() == nullptr
      && implementation->asPathable() == nullptr) {
    !graphElement->forwardEdges.begin()->second.empty();
    vector<GraphEdge>& channelEdges =
        graphElement->forwardEdges.begin()->second;
    const GraphEdge& firstEdge = channelEdges[0];

    ostringstream errorStream;
    errorStream
        << "Cannot make a connection from Graph Element '"
        << graphElement->elementName << "' (channel '" << firstEdge.channel
        << "', type '" << instance->getType() << "') "
        << " to Graph Element '" << firstEdge.next->elementName
        << "' because the implementation is not an ISource or IPathable.";

    throw runtime_error(errorStream.str());
  }

  if (!graphElement->backEdges.empty() && implementation->asSink() == nullptr
      && implementation->asPathable() == nullptr) {
    const auto& firstBackEdge = graphElement->backEdges.begin()->lock();

    ostringstream errorStream;
    errorStream << "Cannot make a connection to Graph Element '"
                << graphElement->elementName << "' with type '"
                << instance->getType() << "' ";

    if (firstBackEdge != nullptr) {
      const auto prevGraphElement = firstBackEdge;
      errorStream << " from Graph Element '" << prevGraphElement->elementName
                  << "'.";
    }

    errorStream << "' because the receiving implementation is not an ISink "
                   "or IPathable.";

    throw runtime_error(errorStream.str());
  }
}

void DataGraph::setNodeFactory(const std::shared_ptr<NodeFactory>& factory) {
  impl->mNodeFactory = factory;
}

void DataGraph::visitGraphElements(
    const Graph::GraphElementVisitor& visitor) const {
  impl->mGraph.visitGraphElements(visitor);
}

}  // namespace maplang
