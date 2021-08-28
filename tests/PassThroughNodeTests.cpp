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

#include <maplang/LambdaPathable.h>

#include <thread>

#include "gtest/gtest.h"
#include "maplang/DataGraph.h"
#include "maplang/FactoriesBuilder.h"
#include "maplang/SimpleSource.h"

using namespace std;

namespace maplang {

TEST(
    WhenAPacketIsSentToAPassThroughNode,
    ItIsPassedThroughFromTheCorrectOutputChannel) {
  const string testChannel = "test channel";

  DataGraph graph(FactoriesBuilder().BuildFactories());

  size_t receivedPacketCount = 0;

  auto passThroughInitParams = R"({
        "outputChannel": "Pass-through output channel"
      })"_json;

  graph.createNode("pass-through", true, true);

  graph.setNodeInstance("pass-through", "pass-through instance");
  graph.setInstanceInitParameters(
      "pass-through instance",
      passThroughInitParams);
  graph.setInstanceType("pass-through instance", "Pass-through");

  auto lambdaSink = make_shared<LambdaPathable>(
      [&receivedPacketCount](const PathablePacket& packet) {
        receivedPacketCount++;
      });

  graph.createNode("test lambda sink", true, false);

  graph.setNodeInstance("test lambda sink", "test lambda sink instance");
  graph.setInstanceImplementation("test lambda sink instance", lambdaSink);

  graph.connect(
      "pass-through",
      "Pass-through output channel",
      "test lambda sink");

  graph.startGraph();

  graph.sendPacket(Packet(), "pass-through");

  usleep(100000);

  ASSERT_EQ(1, receivedPacketCount);
}

}  // namespace maplang
