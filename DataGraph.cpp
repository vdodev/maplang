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

#include "DataGraph.h"
#include <uv.h>

#include <list>
#include <unordered_map>

#include "ISubgraphContext.h"
#include "concurrentqueue.h"
#include "graph/Graph.h"
#include "logging.h"

using namespace std;
using json = nlohmann::json;

namespace dgraph {

struct GraphElement;



struct PushedPacketInfo {
  Packet packet;
  shared_ptr<GraphElement> fromGraphElement;
  shared_ptr<GraphElement> manualSendToGraphElement;  // when enqueued from DataGraph::sendPacket().
  shared_ptr<json> lastSentParameters;

  string channel;
};

class DataGraphImpl final : public enable_shared_from_this<DataGraphImpl> {
 public:
  Graph mGraph;
  const shared_ptr<uv_loop_t> mUvLoop;
  shared_ptr<ISubgraphContext> mSubgraphContext;
  uv_async_t mPacketReadyAsync;
  moodycamel::ConcurrentQueue<PushedPacketInfo> mPacketQueue;

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
  bool setSaveDataForNode(const nlohmann::json& saveData, const INode* node);
  void sendPacketToNode(
      const shared_ptr<GraphElement>& receivingGraphElement,
      const Packet& packet);
  void setupPacketPusherForGraphElement(const shared_ptr<GraphElement>& graphElement);
};

class SubgraphContext : public ISubgraphContext {
 public:
  SubgraphContext(const shared_ptr<DataGraphImpl> impl)
      : mUvLoop(impl->mUvLoop), mImplWeak(impl) {}

  void setSaveDataForNode(const nlohmann::json& saveData,
                          const INode* node) override {
    auto impl = mImplWeak.lock();
    if (impl == nullptr) {
      return;
    }

    impl->setSaveDataForNode(saveData, node);
  }

  shared_ptr<uv_loop_t> getUvLoop() const override { return mUvLoop; }

 private:
  const shared_ptr<uv_loop_t> mUvLoop;
  const weak_ptr<DataGraphImpl> mImplWeak;
};

class GraphPacketPusher : public IPacketPusher {
 public:
  GraphPacketPusher(const shared_ptr<DataGraphImpl>& impl,
                    const shared_ptr<GraphElement>& graphElement)
      : mImpl(impl), mGraphElement(graphElement) {}

  ~GraphPacketPusher() override = default;

  void pushPacket(const Packet* packet, const string& channel) override {
    PushedPacketInfo info;

    Packet packetWithAccumulatedParameters;
    packetWithAccumulatedParameters.parameters = mGraphElement->lastReceivedParameters;
    if (!packet->parameters.empty()) {
      if (packetWithAccumulatedParameters.parameters.empty()) {
        packetWithAccumulatedParameters.parameters = packet->parameters;
      } else {
        packetWithAccumulatedParameters.parameters.insert(packet->parameters.begin(), packet->parameters.end());
      }
    }
    packetWithAccumulatedParameters.buffers = packet->buffers;

    info.packet = move(packetWithAccumulatedParameters);
    info.fromGraphElement = mGraphElement;
    info.channel = channel;

    mImpl->mPacketQueue.enqueue(move(info));
    uv_async_send(&mImpl->mPacketReadyAsync);
  }

 private:
  const shared_ptr<DataGraphImpl> mImpl;
  const shared_ptr<GraphElement> mGraphElement;
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

      const bool propagateParameters = packetInfo.lastSentParameters != nullptr;
      Packet packetWithPropagatedParameters;
      if (propagateParameters) {
        packetWithPropagatedParameters.buffers = move(packetInfo.packet.buffers);
        packetWithPropagatedParameters.parameters = move(*packetInfo.lastSentParameters);
        const json &packetParameters = packetInfo.packet.parameters;
        if (packetWithPropagatedParameters.parameters.empty()) {
          packetWithPropagatedParameters.parameters = packetParameters;
        } else {
          packetWithPropagatedParameters.parameters.insert(packetParameters.begin(), packetParameters.end());
        }
      }

      const Packet& usePacket = propagateParameters ? packetWithPropagatedParameters : packetInfo.packet;
      if (packetInfo.fromGraphElement) {
        bool sentToAny = false;
        for (const auto& nextEdge : packetInfo.fromGraphElement->forwardEdges) {
          if (nextEdge.channel == packetInfo.channel) {
            sentToAny = true;
            sendPacketToNode(nextEdge.otherGraphElement, usePacket);
          }
        }

        if (!sentToAny) {
          logd("Dropped packet from channel %s\n", packetInfo.channel.c_str());
        }
      } else if (packetInfo.manualSendToGraphElement) {
        sendPacketToNode(packetInfo.manualSendToGraphElement, packetWithPropagatedParameters);
      }
    }
  }
}

bool DataGraphImpl::setSaveDataForNode(const nlohmann::json& saveData,
                                       const INode* node) {
  throw runtime_error("TODO: implement");
}

void DataGraphImpl::sendPacketToNode(
    const shared_ptr<GraphElement>& receivingGraphElement,
    const Packet& packet) {
  receivingGraphElement->lastReceivedParameters = packet.parameters;

  const auto pathable = receivingGraphElement->node->asPathable();
  if (pathable) {
    PathablePacket pathablePacket;
    pathablePacket.parameters = packet.parameters;
    pathablePacket.buffers = packet.buffers;
    pathablePacket.packetPusher = receivingGraphElement->pathablePacketPusher;

    pathable->handlePacket(&pathablePacket);
  } else {
    const auto sink = receivingGraphElement->node->asSink();
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

  const bool firstTimeFromNodeWasAdded = !impl->mGraph.hasNode(fromNode, fromPathableId);
  const bool firstTimeToNodeWasAdded = !impl->mGraph.hasNode(toNode, toPathableId);

  impl->mGraph.connect(fromNode, fromChannel, toNode, fromPathableId, toPathableId);

  if (firstTimeFromNodeWasAdded) {
    const auto fromGraphElement = impl->mGraph.getOrCreateGraphElement(fromNode, fromPathableId);
    fromNode->setSubgraphContext(impl->mSubgraphContext);
    impl->setupPacketPusherForGraphElement(fromGraphElement);
  }

  if (firstTimeToNodeWasAdded) {
    const auto toGraphElement = impl->mGraph.getOrCreateGraphElement(toNode, toPathableId);
    toNode->setSubgraphContext(impl->mSubgraphContext);
    impl->setupPacketPusherForGraphElement(toGraphElement);
  }
}

void DataGraphImpl::setupPacketPusherForGraphElement(const shared_ptr<GraphElement>& graphElement) {
  const auto packetPusher = make_shared<GraphPacketPusher>(shared_from_this(), graphElement);
  const auto source = graphElement->node->asSource();
  const auto pathable = graphElement->node->asPathable();

  if (pathable != nullptr) {
    graphElement->pathablePacketPusher = packetPusher;
  } else if (source) {
    source->setPacketPusher(packetPusher);
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
  packetInfo.manualSendToGraphElement = impl->mGraph.getOrCreateGraphElement(toNode, toPathableId);

  impl->mPacketQueue.enqueue(move(packetInfo));
  uv_async_send(&impl->mPacketReadyAsync);
}

}  // namespace dgraph
