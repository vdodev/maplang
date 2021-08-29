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
  shared_ptr<GraphNode> fromNode;
  thread::id queuedFromThreadId;
  string channel;

  // Set when enqueued from DataGraph::sendPacket().
  shared_ptr<GraphNode> manualSendToNode;
};

struct ThreadGroup {
  ThreadGroup(
      const shared_ptr<UvLoopRunner>& uvLoopRunner,
      const shared_ptr<DataGraphImpl>& dataGraphImpl);

  static void packetReadyWrapper(uv_async_t* handle);
  void packetReady();
  void sendPacketToNode(
      const shared_ptr<GraphNode>& receivingNode,
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
  const Factories mFactories;
  unordered_map<string, shared_ptr<ThreadGroup>> mThreadGroups;
  unordered_map<string, shared_ptr<Instance>> mInstances;
  vector<string> mPublicNodeNames;

 public:
  static void packetReadyWrapper(uv_async_t* handle);
  static void logDroppedPacket(
      const shared_ptr<GraphNode>& node,
      const Packet& packet,
      const string& channel);

  DataGraphImpl(const Factories& factories)
      : mFactories(factories) {}

  shared_ptr<ThreadGroup> getOrCreateThreadGroup(const string& name);
  shared_ptr<Instance> getOrCreateInstance(const string& instanceName);
  shared_ptr<Instance> getInstanceForGraphNode(
      const std::shared_ptr<GraphNode>& node) const;
  void validateConnections(const shared_ptr<GraphNode>& node) const;
  void setThreadGroupForInstance(
      const string& instanceName,
      const string& threadGroupName);

  void validateInstanceImplementation(
      const string& graphNode,
      const shared_ptr<Instance>& instance) const;
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
      const weak_ptr<GraphNode>& node)
      : mImpl(impl), mNode(node) {}

  ~GraphPacketPusher() override = default;

  void pushPacket(const Packet& packet, const string& channel) override {
    Packet copy = packet;
    pushPacket(move(copy), channel);
  }

  void pushPacket(Packet&& packet, const string& fromChannel) override {
    const auto fromNode = mNode.lock();
    if (fromNode == nullptr) {
      return;
    }

    Packet packetWithAccumulatedParameters = move(packet);
    const auto lastReceivedParameters = fromNode->lastReceivedParameters;

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
    const auto edgesFromChannelIt = fromNode->forwardEdges.find(fromChannel);
    if (edgesFromChannelIt == fromNode->forwardEdges.end()) {
      DataGraphImpl::logDroppedPacket(fromNode, packet, fromChannel);
      return;
    }

    const vector<GraphEdge>& channelEdges = edgesFromChannelIt->second;

    for (const GraphEdge& graphEdge : channelEdges) {
      const shared_ptr<GraphNode> nextNode = graphEdge.next;
      const shared_ptr<Instance> nextInstance =
          mImpl->getInstanceForGraphNode(nextNode);
      const auto nextNodesThreadGroup =
          mImpl->getOrCreateThreadGroup(nextInstance->getThreadGroupName());
      const auto nextNodesThreadId = nextNodesThreadGroup->mUvLoopThreadId;
      const bool nextNodeIsSameThreadGroup = thisThreadId == nextNodesThreadId;
      const bool pushDirectly =
          nextNodeIsSameThreadGroup
          && graphEdge.sameThreadQueueToTargetType
                 == PacketDeliveryType::PushDirectlyToTarget;

      if (pushDirectly) {
        nextNodesThreadGroup->sendPacketToNode(
            nextNode,
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

    for (const auto& threadGroupPacketPair : asyncThreadGroupPacketPairs) {
      const auto& threadGroup = threadGroupPacketPair.first;
      const auto& packetWithAccumulatedParameters =
          threadGroupPacketPair.second;

      PushedPacketInfo info;
      info.packet = packetWithAccumulatedParameters;
      info.fromNode = fromNode;
      info.channel = fromChannel;
      info.queuedFromThreadId = thisThreadId;

      threadGroup->mPacketQueue.enqueue(move(info));
      uv_async_send(&threadGroup->mPacketReadyAsync);
    }
  }

 private:
  const shared_ptr<DataGraphImpl> mImpl;
  const weak_ptr<GraphNode> mNode;
};

shared_ptr<ThreadGroup> DataGraphImpl::getOrCreateThreadGroup(
    const string& threadGroupName) {
  auto existingIt = mThreadGroups.find(threadGroupName);
  if (existingIt != mThreadGroups.end()) {
    return existingIt->second;
  }

  const auto loopRunner =
      mFactories.uvLoopRunnerFactory->createUvLoopRunner();

  const auto threadGroup =
      make_shared<ThreadGroup>(loopRunner, shared_from_this());
  mThreadGroups.insert(make_pair(threadGroupName, threadGroup));

  return threadGroup;
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

      if (packetInfo.fromNode) {
        for (const auto& nextChannelInfo : packetInfo.fromNode->forwardEdges) {
          const string& channel = nextChannelInfo.first;
          const vector<GraphEdge>& channelEdges = nextChannelInfo.second;

          for (const GraphEdge& nextEdge : channelEdges) {
            const auto nextNode = nextEdge.next;
            const auto nextInstance = impl->getInstanceForGraphNode(nextNode);
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

            if (thisEdgeUsesQueuedPackets && channel == packetInfo.channel) {
              sendPacketToNode(nextNode, packetInfo.packet);
            }
          }
        }
      } else if (packetInfo.manualSendToNode) {
        sendPacketToNode(packetInfo.manualSendToNode, packetInfo.packet);
      }
    }
  }
}

void ThreadGroup::sendPacketToNode(
    const shared_ptr<GraphNode>& receivingNode,
    const Packet& packet) {
  const auto dataGraphImpl = mDataGraphImpl.lock();
  if (dataGraphImpl == nullptr) {
    logw("Dropping packet because DataGraph is gone.");
    return;
  }

  receivingNode->lastReceivedParameters = make_shared<json>(packet.parameters);

  const auto receivingInstance =
      dataGraphImpl->getInstanceForGraphNode(receivingNode);

  if (receivingInstance == nullptr) {
    throw runtime_error(
        "An Instance has not been set for Node '" + receivingNode->name + "'.");
  }

  const auto receivingImplementation = receivingInstance->getImplementation();

  if (receivingImplementation == nullptr) {
    throw runtime_error(
        "Type or Implementation has not been set for Instance '"
        + receivingNode->instanceName + "'.");
  }

  const auto pathable = receivingImplementation->asPathable();

  pathable->handlePacket(PathablePacket(packet, receivingNode->packetPusher));
}

DataGraph::DataGraph(const Factories& factories)
    : impl(new DataGraphImpl(factories)) {}

shared_ptr<GraphNode> DataGraph::createNode(
    const string& name,
    bool allowIncoming,
    bool allowOutgoing) {
  const auto node =
      impl->mGraph.createGraphNode(name, allowIncoming, allowOutgoing);

  node->packetPusher = make_shared<GraphPacketPusher>(impl, node);

  return node;
}

void DataGraph::connect(
    const string& fromNodeName,
    const string& fromChannel,
    const string& toNodeName,
    const PacketDeliveryType& sameThreadQueueToTargetType) {
  if (fromNodeName.empty()) {
    throw runtime_error(
        "fromNodeName must be set - error connecting from node '" + fromNodeName
        + "' to node '" + toNodeName + "'");
  } else if (fromChannel.empty()) {
    throw runtime_error(
        "fromChannel must be set - error connecting from node '" + fromNodeName
        + "' to node '" + toNodeName + "'");
  } else if (toNodeName.empty()) {
    throw runtime_error(
        "toNodeName must be set - error connecting from node '" + fromNodeName
        + "' to node '" + toNodeName + "'");
  }

  const auto fromNode = impl->mGraph.getNodeOrThrow(fromNodeName);
  const auto toNode = impl->mGraph.getNodeOrThrow(toNodeName);

  if (!fromNode->allowsOutgoingConnections) {
    throw runtime_error(
        "Cannot make a connection from '" + fromNodeName + "' to '" + toNodeName
        + "': '" + fromNodeName + "' does not allow outgoing connections.");
  }

  if (!toNode->allowsIncomingConnections) {
    throw runtime_error(
        "Cannot make a connection from '" + fromNodeName + "' to '" + toNodeName
        + "': '" + toNodeName + "' does not allow incoming connections.");
  }

  if (fromNode->instanceName.empty()) {
    throw runtime_error(
        "Error connecting node '" + fromNodeName + "' to '" + toNodeName
        + "': Source node instance must be set before connecting nodes.");
  }

  if (toNode->instanceName.empty()) {
    throw runtime_error(
        "Error connecting node '" + fromNodeName + "' to '" + toNodeName
        + "': Target node instance must be set before connecting nodes.");
  }

  auto& edge = impl->mGraph.connect(fromNodeName, fromChannel, toNodeName);

  edge.sameThreadQueueToTargetType = sameThreadQueueToTargetType;
}

void DataGraph::disconnect(
    const string& fromNodeName,
    const string& fromChannel,
    const string& toNodeName) {
  logi(
      "Disconnecting \"%s\" -> \"%s\", channel=\"%s\"\n",
      fromNodeName.c_str(),
      toNodeName.c_str(),
      fromChannel.c_str());

  impl->mGraph.disconnect(fromNodeName, fromChannel, toNodeName);
}

void DataGraph::sendPacket(const Packet& packet, const string& toNodeName) {
  const shared_ptr<GraphNode> toNode = impl->mGraph.getNodeOrThrow(toNodeName);

  PushedPacketInfo packetInfo;
  packetInfo.packet = packet;
  packetInfo.manualSendToNode = toNode;
  packetInfo.queuedFromThreadId = this_thread::get_id();

  const auto instance = impl->getInstanceForGraphNode(toNode);
  const auto sendToThreadGroup =
      impl->getOrCreateThreadGroup(instance->getThreadGroupName());

  sendToThreadGroup->mPacketQueue.enqueue(move(packetInfo));
  uv_async_send(&sendToThreadGroup->mPacketReadyAsync);
}

void DataGraphImpl::logDroppedPacket(
    const shared_ptr<GraphNode>& node,
    const Packet& packet,
    const string& channel) {
  if (channel == "error") {
    logi("Dropped error packet: %s\n", packet.parameters.dump(2).c_str());
  } else {
    if (channel == "Body Data") {
      logi("here");
    }
    logd(
        "Dropped packet from node '%s' instance '%s', channel '%s'\n",
        node->name.c_str(),
        node->instanceName.c_str(),
        channel.c_str());
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

void DataGraphImpl::validateInstanceImplementation(
    const string& instanceName,
    const shared_ptr<Instance>& instance) const {
  const auto implementation = instance->getImplementation();
  const bool isSource = implementation->asSource() != nullptr;
  const bool isPathable = implementation->asPathable() != nullptr;

  mGraph.visitNodes([isSource, isPathable, instanceName, instance](
                        const shared_ptr<GraphNode>& graphNode) {
    const bool nodePointsToThisInstance =
        graphNode->instanceName == instanceName;

    if (!nodePointsToThisInstance) {
      return;
    }

    if (graphNode->allowsIncomingConnections && !isPathable) {
      ostringstream errorStream;
      errorStream << "GraphNode '" << graphNode->name
                  << "' requires the instance to support incoming connections, "
                     "but Instance '"
                  << instanceName << "' ";

      if (!instance->getType().empty()) {
        errorStream << "with type '" + instance->getType() << "' ";
      } else {
        errorStream << "(with a manually set implementation) ";
      }

      errorStream << "is not an IPathable.";

      throw runtime_error(errorStream.str());
    }

    if (graphNode->allowsOutgoingConnections && !isPathable && !isSource) {
      ostringstream errorStream;
      errorStream << "GraphNode '" << graphNode->name
                  << "' requires the instance to support outgoing connections, "
                     "but Instance '"
                  << instanceName << "' ";

      if (!instance->getType().empty()) {
        errorStream << "with type '" + instance->getType() << "' ";
      } else {
        errorStream << "(with a manually set implementation) ";
      }

      errorStream << "is not an IPathable or an ISource.";

      throw runtime_error(errorStream.str());
    }
  });
}

void DataGraph::setNodeInstance(
    const std::string& nodeName,
    const std::string& instanceName) {
  const shared_ptr<GraphNode> node = impl->mGraph.getNodeOrThrow(nodeName);

  node->instanceName = instanceName;
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

void DataGraph::insertInstanceInitParameters(
    const std::string& instanceName,
    const nlohmann::json& initParameters) {
  if (!initParameters.is_object()) {
    throw runtime_error("initParameters must be an object.");
  }

  const shared_ptr<Instance> instance = impl->getOrCreateInstance(instanceName);
  instance->insertInitParameters(initParameters);
}

void DataGraph::setInstanceType(
    const std::string& instanceName,
    const std::string& typeName) {
  if (typeName.empty()) {
    throw runtime_error(
        "Cannot use empty typeName. Instance: '" + instanceName + "'");
  }

  const shared_ptr<Instance> instance = impl->getOrCreateInstance(instanceName);

  try {
    instance->setType(typeName);
  } catch (exception& e) {
    ostringstream errorStream;
    errorStream << "Error instantiating Instance '" << instanceName
                << "' type '" << typeName << "' initParameters " << setw(2)
                << instance->getInitParameters() << ": " << e.what();

    const string errorString = errorStream.str();
    loge("%s\n", errorString.c_str());

    throw e;
  }

  impl->validateInstanceImplementation(instanceName, instance);
  impl->mInstances[instanceName] = instance;
}

void DataGraph::setInstanceImplementation(
    const std::string& instanceName,
    const std::shared_ptr<IImplementation>& implementation) {
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
  IGroup* group = groupInstance->getImplementation()->asGroup();
  if (group == nullptr) {
    throw runtime_error(
        "Implementation of '" + groupInstanceName
        + "' is not a group. Error while attempting to set Instance '"
        + instanceName + "' implementation to the group's '"
        + groupInterfaceName + "' interface.");
  }

  shared_ptr<IImplementation> groupInterfaceNode =
      group->getInterface(groupInterfaceName);
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

  const auto instance = make_shared<Instance>(mFactories);
  mInstances[instanceName] = instance;
  setThreadGroupForInstance(instanceName, DataGraph::kDefaultThreadGroupName);

  return instance;
}

shared_ptr<Instance> DataGraphImpl::getInstanceForGraphNode(
    const std::shared_ptr<GraphNode>& node) const {
  const string instanceName = node->instanceName;
  if (instanceName.empty()) {
    throw runtime_error(
        "No instance assigned to GraphNode '" + node->name + "'.");
  }

  const auto instanceIt = mInstances.find(instanceName);
  if (instanceIt == mInstances.end()) {
    throw runtime_error(
        "Instance '" + instanceName + "' does not exist. Needed by GraphNode '"
        + node->name + "'.");
  }

  return instanceIt->second;
}

void DataGraphImpl::validateConnections(
    const shared_ptr<GraphNode>& node) const {
  const auto instance = getInstanceForGraphNode(node);
  const auto implementation = instance->getImplementation();
  if (implementation == nullptr) {
    return;
  }

  node->cleanUpEmptyEdges();

  if (!node->forwardEdges.empty() && implementation->asSource() == nullptr
      && implementation->asPathable() == nullptr) {
    !node->forwardEdges.begin()->second.empty();
    vector<GraphEdge>& channelEdges = node->forwardEdges.begin()->second;
    const GraphEdge& firstEdge = channelEdges[0];

    ostringstream errorStream;
    errorStream << "Cannot make a connection from GraphNode '" << node->name
                << "' (channel '" << firstEdge.channel << "'";

    if (!instance->getType().empty()) {
      errorStream << ", type '" << instance->getType() << "'";
    }

    errorStream
        << ") to GraphNode '" << firstEdge.next->name
        << "' because the implementation is not an ISource or IPathable.";

    throw runtime_error(errorStream.str());
  }

  if (!node->backEdges.empty() && implementation->asPathable() == nullptr) {
    const auto& firstBackEdge = node->backEdges.begin()->lock();

    ostringstream errorStream;
    errorStream << "Cannot make a connection to GraphNode '" << node->name
                << "' with type '" << instance->getType() << "' ";

    if (firstBackEdge != nullptr) {
      const auto prevNode = firstBackEdge;
      errorStream << " from GraphNode '" << prevNode->name << "'.";
    }

    errorStream
        << "' because the receiving implementation is not an IPathable.";

    throw runtime_error(errorStream.str());
  }
}

void DataGraph::visitNodes(const Graph::NodeVisitor& visitor) const {
  impl->mGraph.visitNodes(visitor);
}

size_t DataGraph::getInterfaceCount() { return impl->mPublicNodeNames.size(); }

std::string DataGraph::getInterfaceName(size_t nodeIndex) {
  if (nodeIndex >= impl->mPublicNodeNames.size()) {
    throw runtime_error("Interface index is out of bounds.");
  }

  return impl->mPublicNodeNames[nodeIndex];
}

std::shared_ptr<IImplementation> DataGraph::getInterface(
    const std::string& nodeName) {
  const auto nameIt = find(
      impl->mPublicNodeNames.begin(),
      impl->mPublicNodeNames.end(),
      nodeName);

  if (nameIt == impl->mPublicNodeNames.end()) {
    throw runtime_error("No node exists with name '" + nodeName + "'.");
  }

  const auto node = impl->mGraph.getNodeOrThrow(nodeName);
  const auto instance = impl->getInstanceForGraphNode(node);

  return instance->getImplementation();
}

void DataGraph::startGraph() {
  // create uv loop
  // set subgraph contexts
  // set packet pushers
  // start uv loop

  impl->mGraph.visitNodes([this](const shared_ptr<GraphNode>& node) {
    shared_ptr<Instance> instance = impl->getInstanceForGraphNode(node);
    const auto implementation = instance->getImplementation();

    if (implementation == nullptr) {
      ostringstream errorStream;
      errorStream << "Instance '" << node->instanceName
                  << "' was not implemented. Referenced by node '" << node->name
                  << "'.";

      throw runtime_error(errorStream.str());
    }

    const ISource* const source = implementation->asSource();
    const IPathable* const pathable = implementation->asPathable();

    if (node->allowsIncomingConnections && pathable == nullptr) {
      ostringstream errorStream;
      errorStream << "Graph node '" << node->name
                  << "' requires inputs, but instance's '" << node->instanceName
                  << "' type '" << instance->getType()
                  << "' is not an IPathable.";

      throw runtime_error(errorStream.str());
    }

    if (node->allowsOutgoingConnections && source == nullptr
        && pathable == nullptr) {
      ostringstream errorStream;
      errorStream << "Graph node '" << node->name
                  << "' requires outputs, but instance's '"
                  << node->instanceName << "' type '" << instance->getType()
                  << "' is not an ISource or an IPathable.";

      throw runtime_error(errorStream.str());
    }

    instance->setPacketPusherForISources(node->packetPusher);
  });
}

}  // namespace maplang
