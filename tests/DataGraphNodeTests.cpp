/*
 * Copyright 2020 VDO Dev Inc <support@maplang.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <maplang/LambdaSink.h>

#include <thread>

#include "gtest/gtest.h"
#include "maplang/DataGraph.h"
#include "maplang/NodeRegistration.h"
#include "maplang/Util.h"
#include "nodes/DataGraphNode.h"
#include "maplang/LambdaSink.h"

using namespace std;

namespace maplang {

class SimpleSource : public INode, public ISource {
 public:
  void setPacketPusher(const std::shared_ptr<IPacketPusher>& pusher) override {
    mPusher = pusher;
  }

  IPathable* asPathable() override { return nullptr; }
  ISink* asSink() override { return nullptr; }
  ISource* asSource() override { return this; }
  ICohesiveGroup* asGroup() override { return nullptr; }

  void sendPacket(const Packet& packet, const std::string& fromChannel) {
    mPusher->pushPacket(packet, fromChannel);
  }

 private:
  std::shared_ptr<IPacketPusher> mPusher;
};

class LambdaGroup : public INode, public ICohesiveGroup {
 public:
  LambdaGroup(function<void(const Packet& p)>&& onlyNodeLambda)
      : mOnlyNode(make_shared<LambdaSink>(move(onlyNodeLambda))) {}
  ~LambdaGroup() override = default;

  size_t getNodeCount() override { return 1; }
  std::string getNodeName(size_t nodeIndex) override { return "Test Subnode"; }
  std::shared_ptr<INode> getNode(const std::string& nodeName) override {
    return mOnlyNode;
  }

  IPathable* asPathable() override { return nullptr; }
  ISink* asSink() override { return nullptr; }
  ISource* asSource() override { return nullptr; }
  ICohesiveGroup* asGroup() override { return this; }

 private:
  const shared_ptr<INode> mOnlyNode;
};

class PassThroughSourceSink : public INode, public ISource, public ISink {
 public:
  PassThroughSourceSink(const string& sendOnChannel)
      : mSendOnChannel(sendOnChannel) {}

  void setPacketPusher(const std::shared_ptr<IPacketPusher>& pusher) override {
    mPusher = pusher;
  }

  void handlePacket(const Packet& packet) override {
    mPusher->pushPacket(packet, mSendOnChannel);
  }

  IPathable* asPathable() override { return nullptr; }
  ISink* asSink() override { return this; }
  ISource* asSource() override { return this; }
  ICohesiveGroup* asGroup() override { return nullptr; }

 private:
  shared_ptr<IPacketPusher> mPusher;
  const string mSendOnChannel;
};

TEST(WhenADataGraphNodeIsCreated, NothingFails) {
  auto reg = NodeRegistration::defaultRegistration();
  const auto graphNode = reg->createNode("Data Graph", nlohmann::json());
}

TEST(WhenANodeIsCreated, NothingFails) {
  DataGraph graph;

  const string testChannel = "test channel";

  auto reg = NodeRegistration::defaultRegistration();
  const auto graphNode = reg->createNode("Data Graph", nlohmann::json());

  const auto createNodesFeeder = make_shared<SimpleSource>();
  const auto connectNodesFeeder = make_shared<SimpleSource>();

  graph.connect(
      createNodesFeeder,
      testChannel,
      graphNode->asGroup()->getNode(DataGraphNode::kNode_CreateNode));
  graph.connect(
      connectNodesFeeder,
      testChannel,
      graphNode->asGroup()->getNode(DataGraphNode::kNode_Connect));

  nlohmann::json receivedParameters;
  reg->registerNodeFactory(
      "DataGraphNodeTest Lambda",
      [&receivedParameters](const nlohmann::json& initParameters) {
        return make_shared<LambdaSink>(
            [&receivedParameters](const Packet& packet) {
              receivedParameters = packet.parameters;
            });
      });

  Packet createNodesPacket = packetWithParameters(R"({
    "nodesToCreate": {
      "Send Once 1": {
        "nodeImplementation": "Send Once",
        "initParameters": {
          "value": 5
        }
      },
      "Test Receiver": {
        "nodeImplementation": "DataGraphNodeTest Lambda"
      }
    }
  })"_json);

  Packet connectNodesPacket = packetWithParameters(R"([
    {
      "fromNode": "Send Once 1",
      "channel": "initialized",
      "toNode": "Test Receiver"
    }
  ])"_json);

  createNodesFeeder->sendPacket(createNodesPacket, testChannel);
  connectNodesFeeder->sendPacket(connectNodesPacket, testChannel);

  usleep(100000);

  ASSERT_TRUE(receivedParameters.contains("value"));
  ASSERT_EQ(5, receivedParameters["value"].get<uint32_t>());
}

TEST(WhenAPacketIsSentDirectly, ItIsReceived) {
  DataGraph graph;

  const string testChannel = "test channel";

  auto reg = NodeRegistration::defaultRegistration();
  const auto graphNode = reg->createNode("Data Graph", nlohmann::json());

  const auto createNodesFeeder = make_shared<SimpleSource>();
  const auto connectNodesFeeder = make_shared<SimpleSource>();
  const auto sendPacketFeeder = make_shared<SimpleSource>();

  graph.connect(
      createNodesFeeder,
      testChannel,
      graphNode->asGroup()->getNode(DataGraphNode::kNode_CreateNode));
  graph.connect(
      connectNodesFeeder,
      testChannel,
      graphNode->asGroup()->getNode(DataGraphNode::kNode_Connect));
  graph.connect(
      sendPacketFeeder,
      testChannel,
      graphNode->asGroup()->getNode(DataGraphNode::kNode_SendPacketToNode));

  nlohmann::json receivedParameters;
  reg->registerNodeFactory(
      "DataGraphNodeTest Lambda",
      [&receivedParameters](const nlohmann::json& initParameters) {
        return make_shared<LambdaSink>(
            [&receivedParameters](const Packet& packet) {
              receivedParameters = packet.parameters;
            });
      });

  Packet createNodesPacket = packetWithParameters(R"({
    "nodesToCreate": {
      "Add Dummy Parameter": {
        "nodeImplementation": "Add Parameters",
        "initParameters": {
          "value": 5
        }
      },
      "Test Receiver": {
        "nodeImplementation": "DataGraphNodeTest Lambda"
      }
    }
  })"_json);

  Packet connectNodesPacket = packetWithParameters(R"([
    {
      "fromNode": "Add Dummy Parameter",
      "channel": "Added Parameters",
      "toNode": "Test Receiver"
    }
  ])"_json);

  Packet wrappedPacketForParameterAdder = packetWithParameters(R"({
    "toNode": "Add Dummy Parameter",
    "packetParameters": {
      "originalValue": 4
    }
  })"_json);

  createNodesFeeder->sendPacket(createNodesPacket, testChannel);
  connectNodesFeeder->sendPacket(connectNodesPacket, testChannel);
  sendPacketFeeder->sendPacket(wrappedPacketForParameterAdder, testChannel);

  usleep(100000);

  ASSERT_TRUE(receivedParameters.contains("originalValue"));
  ASSERT_EQ(4, receivedParameters["originalValue"].get<uint32_t>());
}

TEST(WhenAConnectionIsMadeToAGroupsSubNodeAndAPacketIsSent, ItIsReceived) {
  DataGraph graph;

  const string testChannel = "test channel";

  auto reg = NodeRegistration::defaultRegistration();
  const auto graphNode = reg->createNode("Data Graph", nlohmann::json());

  const auto createNodesFeeder = make_shared<SimpleSource>();
  const auto connectNodesFeeder = make_shared<SimpleSource>();
  const auto sendPacketFeeder = make_shared<SimpleSource>();

  graph.connect(
      createNodesFeeder,
      testChannel,
      graphNode->asGroup()->getNode(DataGraphNode::kNode_CreateNode));
  graph.connect(
      connectNodesFeeder,
      testChannel,
      graphNode->asGroup()->getNode(DataGraphNode::kNode_Connect));
  graph.connect(
      sendPacketFeeder,
      testChannel,
      graphNode->asGroup()->getNode(DataGraphNode::kNode_SendPacketToNode));

  nlohmann::json receivedParameters;
  reg->registerNodeFactory(
      "DataGraphNodeTest LambdaGroup",
      [&receivedParameters](const nlohmann::json& initParameters) {
        return make_shared<LambdaGroup>(
            [&receivedParameters](const Packet& packet) {
              receivedParameters = packet.parameters;
            });
      });

  Packet createNodesPacket = packetWithParameters(R"({
    "nodesToCreate": {
      "Add Dummy Parameter": {
        "nodeImplementation": "Add Parameters",
        "initParameters": {
          "value": 5
        }
      },
      "Test Receiver": {
        "nodeImplementation": "DataGraphNodeTest LambdaGroup"
      }
    }
  })"_json);

  Packet connectNodesPacket = packetWithParameters(R"([
    {
      "fromNode": "Add Dummy Parameter",
      "channel": "Added Parameters",
      "toNode": ["Test Receiver", "Test Subnode"]
    }
  ])"_json);

  Packet wrappedPacketForParameterAdder = packetWithParameters(R"({
    "toNode": "Add Dummy Parameter",
    "packetParameters": {
      "originalValue": 4
    }
  })"_json);

  createNodesFeeder->sendPacket(createNodesPacket, testChannel);
  connectNodesFeeder->sendPacket(connectNodesPacket, testChannel);
  sendPacketFeeder->sendPacket(wrappedPacketForParameterAdder, testChannel);

  usleep(100000);

  ASSERT_TRUE(receivedParameters.contains("originalValue"));
  ASSERT_EQ(4, receivedParameters["originalValue"].get<uint32_t>());
}

}  // namespace maplang
