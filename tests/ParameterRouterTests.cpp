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

#include <functional>

#include "gtest/gtest.h"
#include "maplang/FactoriesBuilder.h"
#include "maplang/GraphBuilder.h"
#include "maplang/LambdaPathable.h"
#include "nodes/ParameterExtractor.h"

using namespace std;
using namespace nlohmann;

namespace maplang {

class NoopPacketPusher : public IPacketPusher {
 public:
  void pushPacket(const Packet& packet, const std::string& channelName)
      override {}

  void pushPacket(Packet&& packet, const std::string& channelName) override {}
};

class ParameterRouterTests : public testing::Test {
 public:
  ParameterRouterTests()
      : mFactories(
          FactoriesBuilder().BuildFactories()) {}

  const std::shared_ptr<const IFactories> mFactories;
};

TEST_F(ParameterRouterTests, WhenAParameterRouterGetsAValidValue_ItUsesThatValueAsTheChannel) {
  const auto graph = buildDataGraph(mFactories, R"(
    strict digraph ParameterRouterTest {
      "Parameter Router" [instance="Parameter Router Instance" allowIncoming=true, allowOutgoing=true]
      "Test Sink" [instance="Test Sink Instance" allowIncoming=true]

      "Parameter Router" -> "Test Sink" [label="value1AsChannel"]
    }
  )");

  implementDataGraph(graph, R"({
    "Parameter Router Instance": {
      "type": "Parameter Router",
      "initParameters": {
        "routingKey": "/someId"
      }
    }
  })");

  size_t receivedPacketCount = 0;
  graph->setInstanceImplementation(
      "Test Sink Instance",
      make_shared<LambdaPathable>(
          [&receivedPacketCount](const PathablePacket& pathablePacket) {
            receivedPacketCount++;
          }));

  graph->startGraph();

  Packet packet;
  packet.parameters = R"({
    "someId": "value1AsChannel",
    "key2": [ 0, 1, 2 ]
  })"_json;

  graph->sendPacket(packet, "Parameter Router");

  usleep(10000);

  ASSERT_EQ(1, receivedPacketCount);
}

TEST_F(ParameterRouterTests, WhenAParameterRouterHasIncorrectInitParameters_ItThrows) {
  const auto graph = buildDataGraph(mFactories, R"(
    strict digraph ParameterRouterTest {
      "Parameter Router" [instance="Parameter Router Instance" allowIncoming=true, allowOutgoing=true]
      "Test Sink" [instance="Test Sink Instance" allowIncoming=true]

      "Parameter Router" -> "Test Sink" [label="value1AsChannel"]
    }
  )");

  EXPECT_ANY_THROW(implementDataGraph(graph, R"({
    "Parameter Router Instance": {
      "type": "Parameter Router",
      "initParameters": {
        "routingKey_INVALID": "/someId"
      }
    }
  })"));
}

TEST_F(ParameterRouterTests, WhenAParameterRouterDoesntGetTheRoutingKey_ItThrows) {
  Packet packet;
  packet.parameters = R"({
    "someId_INVALID": "value1AsChannel",
    "key2": [ 0, 1, 2 ]
  })"_json;

  const auto router =
      mFactories->GetImplementationFactory()->createImplementation(
          "Parameter Router",
          R"({ "routingKey": "/someId" })"_json);

  EXPECT_ANY_THROW(router->asPathable()->handlePacket(
      PathablePacket(packet, make_shared<NoopPacketPusher>())));
}

TEST_F(ParameterRouterTests, WhenAParameterRouterGetsAnObjectValue_ItThrows) {
  Packet packet;
  packet.parameters = R"({
    "someId": { "anotherKey": "value1AsChannel" },
    "key2": [ 0, 1, 2 ]
  })"_json;

  const auto router =
      mFactories->GetImplementationFactory()->createImplementation(
          "Parameter Router",
          R"({ "routingKey": "/someId" })"_json);

  EXPECT_ANY_THROW(router->asPathable()->handlePacket(
      PathablePacket(packet, make_shared<NoopPacketPusher>())));
}

}  // namespace maplang
