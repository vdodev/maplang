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
#include "nodes/ParameterExtractor.h"

using namespace std;
using namespace nlohmann;

namespace maplang {

class LambdaPacketPusher : public IPacketPusher {
 public:
  LambdaPacketPusher(function<void(const Packet& packet, const string& channel)>&& onPacket)
      : mOnPacket(move(onPacket)) {}

  ~LambdaPacketPusher() override = default;

  void pushPacket(const Packet& packet, const string& channel) override { mOnPacket(packet, channel); }

  void pushPacket(Packet&& packet, const string& channel) override { mOnPacket(packet, channel); }

 private:
  function<void(const Packet& packet, const string& channel)> mOnPacket;
};

TEST(WhenAParameterExtractorIsGivenAValidKey, ItOnlyExtractsThatKeysValue) {
  auto extractor = make_shared<ParameterExtractor>(R"({
    "extractParameter": "key3"
  })"_json);

  json objectToExtract = R"({
    "keyA": 1,
    "keyB": "two",
    "keyC": [ 3 ],
    "keyD": {
      "keyI": "x"
    }
  })";

  Packet packet;
  packet.parameters = R"({
    "key1": "value1",
    "key2": [ 0, 1, 2 ]
  })"_json;

  packet.parameters["key3"] = objectToExtract;

  PathablePacket pathablePacket(packet,
      make_shared<LambdaPacketPusher>([&objectToExtract](const Packet& packet, const string& channel) {
        ASSERT_EQ(packet.parameters, objectToExtract);
      }));

  extractor->handlePacket(pathablePacket);
}

}  // namespace maplang
