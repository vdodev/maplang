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

#include "nodes/DataGraphNode.h"

#include <sstream>

#include "maplang/DataGraph.h"
#include "maplang/Errors.h"
#include "maplang/JsonUtil.h"
#include "maplang/NodeRegistration.h"

using namespace nlohmann;
using namespace std;

namespace maplang {

const string DataGraphNode::kNode_Connect = "Connect";
const string DataGraphNode::kNode_Disconnect = "Disconnect";
const string DataGraphNode::kNode_CreateNode = "Create Node";
const string DataGraphNode::kNode_SendPacketToNode = "Send Packet To Node";

const string DataGraphNode::kInputParam_FromNode = "fromNode";
const string DataGraphNode::kInputParam_FromPathableId = "fromPathableId";
const string DataGraphNode::kInputParam_ToNode = "toNode";
const string DataGraphNode::kInputParam_ToPathableId = "toPathableId";
const string DataGraphNode::kInputParam_Channel = "channel";
const string DataGraphNode::kInputParam_PacketDeliveryType =
    "packetDeliveryType";
const string DataGraphNode::kInputParam_InitParameters = "initParameters";
const string DataGraphNode::kInputParam_PacketParameters = "packetParameters";
const string DataGraphNode::kInputParam_NodesToCreate = "nodesToCreate";
const string DataGraphNode::kInputParam_NodeImplementation =
    "nodeImplementation";

struct DataGraphNode::Impl final {
  Impl() : dataGraph(make_shared<DataGraph>()) {}

  unordered_map<string, shared_ptr<INode>> createdNodes;
  const shared_ptr<DataGraph> dataGraph;
};

static optional<shared_ptr<INode>> getNode(
    const json& nameOrGroupPath,
    const unordered_map<string, shared_ptr<INode>>& nodes,
    const shared_ptr<IPacketPusher>& errorPusher) {
  if (nameOrGroupPath.is_string()) {
    const string nodeName = nameOrGroupPath.get<string>();
    const auto it = nodes.find(nodeName);
    if (it == nodes.end()) {
      ostringstream message;
      message << "Node '" << nodeName << "' is not present.";

      sendErrorPacket(errorPusher, "Cannot get Node.", message.str());

      return {};
    }

    return it->second;
  } else if (nameOrGroupPath.is_array()) {
    shared_ptr<INode> currentNode;
    int nameIndex = 0;
    for (auto nameIt = nameOrGroupPath.begin(); nameIt != nameOrGroupPath.end();
         nameIt++, nameIndex++) {
      const string nodeName = nameIt->get<string>();

      if (currentNode == nullptr) {
        auto nodeIt = nodes.find(nodeName);
        if (nodeIt != nodes.end()) {
          currentNode = nodeIt->second;
        }
      } else {
        auto group = currentNode->asGroup();

        if (group != nullptr) {
          try {
            currentNode = group->getNode(nodeName);
          } catch (const exception& ex) {
            currentNode.reset();
          }
        }
      }

      if (currentNode == nullptr) {
        ostringstream message;
        message << "Node ";

        for (int i = 0; i <= nameIndex; i++) {
          message << "'" << nameOrGroupPath[i].get<string>() << "'";

          if (i < nameIndex - 1) {
            message << " -> ";
          }
        }
        message << " is not present.";

        sendErrorPacket(errorPusher, "Cannot get Node.", message.str());

        return {};
      }
    }

    return currentNode;
  } else {
    ostringstream message;
    message << "Node key is unexpected type '" << nameOrGroupPath.type_name()
            << "'.";
    sendErrorPacket(errorPusher, "Cannot get Node.", message.str());

    return {};
  }
}

class ConnectNodes final : public INode, public IPathable {
 public:
  ConnectNodes(const weak_ptr<DataGraphNode::Impl> impl) : mWeakImpl(impl) {}
  ~ConnectNodes() override = default;

  void handlePacket(const PathablePacket& incomingPacket) override {
    const auto& params = incomingPacket.packet.parameters;
    const auto& pusher = incomingPacket.packetPusher;

    if (params.is_object()) {
      addConnection(params, pusher);
    } else if (params.is_array()) {
      for (auto it = params.begin(); it != params.end(); it++) {
        addConnection(*it, pusher);
      }
    } else {
      throw invalid_argument(
          string("Parameters is unexpected type: ") + params.type_name());
    }
  }

  IPathable* asPathable() override { return this; }
  ISource* asSource() override { return nullptr; }
  ISink* asSink() override { return nullptr; }
  ICohesiveGroup* asGroup() override { return nullptr; }

 private:
  void addConnection(
      const nlohmann::json& connectionInfo,
      const shared_ptr<IPacketPusher>& errorPusher) {
    const string fromChannel =
        connectionInfo[DataGraphNode::kInputParam_Channel].get<string>();
    const string fromPathableId = jsonGetOr(
        connectionInfo,
        DataGraphNode::kInputParam_FromPathableId,
        string(""));
    const string toPathableId = jsonGetOr(
        connectionInfo,
        DataGraphNode::kInputParam_ToPathableId,
        string(""));
    const PacketDeliveryType packetDeliveryType = jsonGetOr(
        connectionInfo,
        DataGraphNode::kInputParam_PacketDeliveryType,
        PacketDeliveryType::PushDirectlyToTarget);

    const auto impl = mWeakImpl.lock();
    if (impl == nullptr) {
      return;
    }

    const auto fromNode = getNode(
        connectionInfo[DataGraphNode::kInputParam_FromNode],
        impl->createdNodes,
        errorPusher);
    const auto toNode = getNode(
        connectionInfo[DataGraphNode::kInputParam_ToNode],
        impl->createdNodes,
        errorPusher);

    if (!fromNode.has_value() || !toNode.has_value()) {
      return;  // error already sent by getNode().
    }

    impl->dataGraph->connect(
        *fromNode,
        fromChannel,
        *toNode,
        fromPathableId,
        toPathableId,
        packetDeliveryType);
  }

 private:
  const weak_ptr<DataGraphNode::Impl> mWeakImpl;
};

class DisconnectNodes final : public INode, public IPathable {
 public:
  DisconnectNodes(const weak_ptr<DataGraphNode::Impl> impl) : mWeakImpl(impl) {}
  ~DisconnectNodes() override = default;

  void handlePacket(const PathablePacket& incomingPacket) override {
    const auto& params = incomingPacket.packet.parameters;
    const auto& pusher = incomingPacket.packetPusher;

    if (params.is_object()) {
      disconnectNodes(params, pusher);
    } else if (params.is_array()) {
      for (auto it = params.begin(); it != params.end(); it++) {
        disconnectNodes(*it, pusher);
      }
    } else {
      throw invalid_argument(
          string("Parameters is unexpected type: ") + params.type_name());
    }
  }

  IPathable* asPathable() override { return this; }
  ISource* asSource() override { return nullptr; }
  ISink* asSink() override { return nullptr; }
  ICohesiveGroup* asGroup() override { return nullptr; }

 private:
  void disconnectNodes(
      const nlohmann::json& connectionInfo,
      const shared_ptr<IPacketPusher>& errorPusher) {
    const string fromChannel =
        connectionInfo[DataGraphNode::kInputParam_Channel].get<string>();
    const string fromPathableId = jsonGetOr(
        connectionInfo,
        DataGraphNode::kInputParam_FromPathableId,
        string(""));
    const string toPathableId = jsonGetOr(
        connectionInfo,
        DataGraphNode::kInputParam_ToPathableId,
        string(""));

    const auto impl = mWeakImpl.lock();
    if (impl == nullptr) {
      return;
    }

    const auto fromNode = getNode(
        connectionInfo[DataGraphNode::kInputParam_FromNode],
        impl->createdNodes,
        errorPusher);
    const auto toNode = getNode(
        connectionInfo[DataGraphNode::kInputParam_ToNode],
        impl->createdNodes,
        errorPusher);

    if (!fromNode.has_value() || !toNode.has_value()) {
      return;  // error already sent by getNode().
    }

    impl->dataGraph->disconnect(
        *fromNode,
        fromChannel,
        *toNode,
        fromPathableId,
        toPathableId);
  }

 private:
  const weak_ptr<DataGraphNode::Impl> mWeakImpl;
};

class SendToNode final : public INode, public IPathable {
 public:
  SendToNode(const weak_ptr<DataGraphNode::Impl> impl) : mWeakImpl(impl) {}
  ~SendToNode() override = default;

  void handlePacket(const PathablePacket& incomingPacket) override {
    const auto& params = incomingPacket.packet.parameters;
    const auto& pusher = incomingPacket.packetPusher;

    const string toPathableId =
        jsonGetOr(params, DataGraphNode::kInputParam_ToPathableId, string(""));

    const auto impl = mWeakImpl.lock();
    if (impl == nullptr) {
      return;
    }

    const auto toNode = getNode(
        params[DataGraphNode::kInputParam_ToNode],
        impl->createdNodes,
        pusher);

    if (!toNode.has_value()) {
      return;  // error already sent by getNode().
    }

    Packet packetToSend;
    packetToSend.buffers = incomingPacket.packet.buffers;
    packetToSend.parameters =
        params[DataGraphNode::kInputParam_PacketParameters];

    impl->dataGraph->sendPacket(packetToSend, *toNode, toPathableId);
  }

  IPathable* asPathable() override { return this; }
  ISource* asSource() override { return nullptr; }
  ISink* asSink() override { return nullptr; }
  ICohesiveGroup* asGroup() override { return nullptr; }

 private:
  const weak_ptr<DataGraphNode::Impl> mWeakImpl;
};

class CreateNode final : public INode, public IPathable {
 public:
  CreateNode(const weak_ptr<DataGraphNode::Impl> impl) : mWeakImpl(impl) {}
  ~CreateNode() override = default;

  void handlePacket(const PathablePacket& incomingPacket) override {
    const auto& nodesToCreate =
        incomingPacket.packet
            .parameters[DataGraphNode::kInputParam_NodesToCreate];
    const auto& pusher = incomingPacket.packetPusher;

    for (auto it = nodesToCreate.begin(); it != nodesToCreate.end(); it++) {
      createNode(it.key(), *it, pusher);
    }
  }

  IPathable* asPathable() override { return this; }
  ISource* asSource() override { return nullptr; }
  ISink* asSink() override { return nullptr; }
  ICohesiveGroup* asGroup() override { return nullptr; }

 private:
  void createNode(
      const string& nodeName,
      const nlohmann::json& nodeInfo,
      const shared_ptr<IPacketPusher>& errorPusher) {
    auto reg = NodeRegistration::defaultRegistration();

    const json initParameters =
        nodeInfo.contains(DataGraphNode::kInputParam_InitParameters)
            ? nodeInfo[DataGraphNode::kInputParam_InitParameters]
            : json();

    const string nodeImplementation =
        nodeInfo[DataGraphNode::kInputParam_NodeImplementation].get<string>();

    const auto impl = mWeakImpl.lock();
    if (!impl) {
      return;
    }

    try {
      impl->createdNodes[nodeName] =
          reg->createNode(nodeImplementation, initParameters);
    } catch (const exception& ex) {
      sendErrorPacket(errorPusher, ex);
    }
  }

 private:
  const weak_ptr<DataGraphNode::Impl> mWeakImpl;
};

DataGraphNode::DataGraphNode() : mImpl(make_shared<Impl>()) {
  mNodes[kNode_Connect] = make_shared<ConnectNodes>(mImpl);
  mNodes[kNode_Disconnect] = make_shared<DisconnectNodes>(mImpl);
  mNodes[kNode_CreateNode] = make_shared<CreateNode>(mImpl);
  mNodes[kNode_SendPacketToNode] = make_shared<SendToNode>(mImpl);
}

size_t DataGraphNode::getNodeCount() { return mNodes.size(); }

std::string DataGraphNode::getNodeName(size_t nodeIndex) {
  size_t i = 0;
  for (auto it = mNodes.begin(); it != mNodes.end(); it++, i++) {
    if (i == nodeIndex) {
      return it->first;
    }
  }

  throw invalid_argument("nodeIndex is out of range");
}

std::shared_ptr<maplang::INode> DataGraphNode::getNode(
    const std::string& nodeName) {
  return mNodes[nodeName];
}

}  // namespace maplang
