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
#include "nodes/AddParametersNode.h"
#include "maplang/LambdaPacketPusher.h"

using namespace std;
using namespace nlohmann;

namespace maplang {

TEST(WhenAParameterDoesNotExist, ItIsAdded) {
  auto parameterAdder = make_shared<AddParametersNode>(R"({
    "newParameter": "testStringValue"
  })"_json);

  Packet incomingPacket;
  incomingPacket.parameters = R"({
    "key1": "value1",
    "key2": [ 0, 1, 2 ]
  })"_json;

  PathablePacket pathablePacket(
      incomingPacket,
      make_shared<LambdaPacketPusher>(
          [](const Packet& packet, const string& channel) {
            ASSERT_TRUE(packet.parameters.contains("newParameter"));

            const auto& newParameter = packet.parameters["newParameter"];
            ASSERT_TRUE(newParameter.is_string());
            ASSERT_EQ("testStringValue", newParameter.get<string>());
          }));

  parameterAdder->handlePacket(pathablePacket);
}

TEST(WhenAParameterExists, ItIsReplaced) {
  auto parameterAdder = make_shared<AddParametersNode>(R"({
    "newParameter": "new value"
  })"_json);

  Packet incomingPacket;
  incomingPacket.parameters = R"({
    "key1": "value1",
    "key2": [ 0, 1, 2 ],
    "newParameter": "old value"
  })"_json;

  PathablePacket pathablePacket(
      incomingPacket,
      make_shared<LambdaPacketPusher>(
          [](const Packet& packet, const string& channel) {
            ASSERT_TRUE(packet.parameters.contains("newParameter"));

            const auto& newParameter = packet.parameters["newParameter"];
            ASSERT_TRUE(newParameter.is_string());
            ASSERT_EQ("new value", newParameter.get<string>());
          }));

  parameterAdder->handlePacket(pathablePacket);
}

}  // namespace maplang
