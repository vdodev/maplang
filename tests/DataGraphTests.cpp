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

#include "maplang/DataGraph.h"
#include "maplang/UvLoopRunner.h"
#include "gtest/gtest.h"

using namespace std;

namespace maplang {

class LambdaSink : public INode, public ISink {
 public:
  LambdaSink(function<void (const Packet* packet)>&& onPacket) : mOnPacket(move(onPacket)) {}

  void handlePacket(const Packet* packet) override {
    mOnPacket(packet);
  }

  IPathable *asPathable() override { return nullptr;}
  ISink *asSink() override { return this; }
  ISource *asSource() override { return nullptr; }

 private:
  function<void (const Packet* packet)> mOnPacket;
};

TEST(WhenSendPacketIsCalledOnce, ThenOnePacketIsDeliveredToTheSink) {
  UvLoopRunner uvLoopRunner;

  DataGraph graph(uvLoopRunner.getLoop());

  Packet packet;
  size_t receivedPacketCount = 0;
  auto lambdaSink = make_shared<LambdaSink>([&receivedPacketCount](const Packet* packet) {
    receivedPacketCount++;
  });

  graph.sendPacket(&packet, lambdaSink);

  usleep(1000);
  uv_stop(uvLoopRunner.getLoop().get());
  uvLoopRunner.waitForExit();

  ASSERT_EQ(1, receivedPacketCount);
}

}  // namespace maplang
