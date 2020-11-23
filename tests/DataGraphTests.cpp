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
#include "maplang/UvLoopRunner.h"

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

TEST(WhenSendPacketIsCalledOnce, ThenOnePacketIsDeliveredToTheSink) {
  UvLoopRunner uvLoopRunner;

  DataGraph graph(uvLoopRunner.getLoop());

  Packet packet;
  size_t receivedPacketCount = 0;
  auto lambdaSink = make_shared<LambdaSink>(
      [&receivedPacketCount](const Packet& packet) { receivedPacketCount++; });

  graph.sendPacket(packet, lambdaSink);

  usleep(1000);
  uv_stop(uvLoopRunner.getLoop().get());
  uvLoopRunner.waitForExit();

  ASSERT_EQ(1, receivedPacketCount);
}

TEST(WhenAPacketIsSentDirectly, ItArrives) {
  UvLoopRunner uvLoopRunner;
  const string testChannel = "test channel";

  DataGraph graph(uvLoopRunner.getLoop());

  Packet packet;
  size_t receivedPacketCount = 0;

  auto source = make_shared<SimpleSource>();
  thread::id packetReceivedOnThreadId;

  auto lambdaSink = make_shared<LambdaSink>(
      [&receivedPacketCount, &packetReceivedOnThreadId](const Packet& packet) {
        packetReceivedOnThreadId = this_thread::get_id();
        receivedPacketCount++;
      });

  graph.connect(source, testChannel, lambdaSink);
  source->sendPacket(Packet(), testChannel);

  uv_stop(uvLoopRunner.getLoop().get());
  uvLoopRunner.waitForExit();

  ASSERT_EQ(this_thread::get_id(), packetReceivedOnThreadId);
  ASSERT_EQ(1, receivedPacketCount);
}

TEST(WhenAPacketIsSentUsingAsyncQueueing, ItArrives) {
  UvLoopRunner uvLoopRunner;
  const string testChannel = "test channel";

  DataGraph graph(uvLoopRunner.getLoop());

  Packet packet;
  size_t receivedPacketCount = 0;

  thread::id packetReceivedOnThreadId;
  auto source = make_shared<SimpleSource>();
  auto lambdaSink = make_shared<LambdaSink>(
      [&receivedPacketCount, &packetReceivedOnThreadId](const Packet& packet) {
        packetReceivedOnThreadId = this_thread::get_id();
        receivedPacketCount++;
      });

  graph.connect(
      source,
      testChannel,
      lambdaSink,
      "",
      "",
      PacketDeliveryType::AlwaysQueue);

  source->sendPacket(Packet(), testChannel);
  usleep(1000);

  // Pushed onto the graph's libuv thread.
  ASSERT_NE(this_thread::get_id(), packetReceivedOnThreadId);
  uv_stop(uvLoopRunner.getLoop().get());
  uvLoopRunner.waitForExit();

  ASSERT_EQ(1, receivedPacketCount);
}

TEST(WhenAPacketIsSentUsingDirectAndAsyncQueueing, ItArrives) {
  UvLoopRunner uvLoopRunner;
  const string testChannel = "test channel";

  DataGraph graph(uvLoopRunner.getLoop());

  Packet packet;
  size_t receivedPacketCount = 0;

  thread::id directThreadId;
  thread::id asyncThreadId;
  auto source = make_shared<SimpleSource>();
  auto lambdaSinkDirect = make_shared<LambdaSink>(
      [&receivedPacketCount, &directThreadId](const Packet& packet) {
        directThreadId = this_thread::get_id();
        receivedPacketCount++;
      });

  auto lambdaSinkAsync = make_shared<LambdaSink>(
      [&receivedPacketCount, &asyncThreadId](const Packet& packet) {
        asyncThreadId = this_thread::get_id();
        receivedPacketCount++;
      });

  graph.connect(
      source,
      testChannel,
      lambdaSinkDirect,
      "",
      "",
      PacketDeliveryType::PushDirectlyToTarget);

  graph.connect(
      source,
      testChannel,
      lambdaSinkAsync,
      "",
      "",
      PacketDeliveryType::AlwaysQueue);

  source->sendPacket(Packet(), testChannel);
  usleep(1000);

  uv_stop(uvLoopRunner.getLoop().get());
  uvLoopRunner.waitForExit();

  ASSERT_EQ(2, receivedPacketCount);

  ASSERT_EQ(this_thread::get_id(), directThreadId);
  ASSERT_NE(this_thread::get_id(), asyncThreadId);
}

}  // namespace maplang
