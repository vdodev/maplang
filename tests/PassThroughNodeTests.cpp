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

TEST(
    WhenAPacketIsSentToAPassThroughNode,
    ItIsPassedThroughFromTheCorrectOutputChannel) {
  const string testChannel = "test channel";

  DataGraph graph;

  size_t receivedPacketCount = 0;

  auto reg = NodeRegistration::defaultRegistration();

  auto source = make_shared<SimpleSource>();
  auto passThrough = reg->createNode("Pass-through", R"({
    "outputChannel": "Pass-through output channel"
  })"_json);

  auto lambdaSink = make_shared<LambdaSink>(
      [&receivedPacketCount](const Packet& packet) { receivedPacketCount++; });

  graph.connect(source, testChannel, passThrough);
  graph.connect(passThrough, "Pass-through output channel", lambdaSink);

  source->sendPacket(Packet(), testChannel);

  usleep(100000);

  ASSERT_EQ(1, receivedPacketCount);
}

}  // namespace maplang
