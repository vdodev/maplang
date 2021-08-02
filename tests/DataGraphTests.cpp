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

using namespace std;

namespace maplang {

class SimpleSource : public IImplementation, public ISource {
 public:
  void setPacketPusher(const std::shared_ptr<IPacketPusher>& pusher) override {
    mPusher = pusher;
  }

  IPathable* asPathable() override { return nullptr; }
  ISource* asSource() override { return this; }
  IGroup* asGroup() override { return nullptr; }

  void sendPacket(const Packet& packet, const std::string& fromChannel) {
    mPusher->pushPacket(packet, fromChannel);
  }

 private:
  std::shared_ptr<IPacketPusher> mPusher;
};

TEST(WhenSendPacketIsCalledOnce, ThenOnePacketIsDeliveredToTheSink) {
  DataGraph graph;

  size_t receivedPacketCount = 0;
  auto lambdaSink = make_shared<LambdaPathable>(
      [&receivedPacketCount](const PathablePacket& packet) {
        receivedPacketCount++;
      });

  graph.createNode("sink", false, true);
  graph.setNodeInstance("sink", "sink-instance");
  graph.setInstanceImplementation("sink-instance", lambdaSink);

  graph.sendPacket(Packet(), "sink");

  usleep(100000);

  ASSERT_EQ(1, receivedPacketCount);
}

TEST(WhenAPacketIsSentDirectlyFromADifferentThread, ItArrives) {
  const string testChannel = "test channel";

  DataGraph graph;

  size_t receivedPacketCount = 0;

  auto source = make_shared<SimpleSource>();
  thread::id packetReceivedOnThreadId;

  auto lambdaSink = make_shared<LambdaPathable>(
      [&receivedPacketCount,
       &packetReceivedOnThreadId](const PathablePacket& packet) {
        packetReceivedOnThreadId = this_thread::get_id();
        receivedPacketCount++;
      });

  graph.createNode("source", false, true);
  graph.createNode("sink", true, false);

  graph.setNodeInstance("source", "source-instance");
  graph.setNodeInstance("sink", "sink-instance");

  graph.connect("source", testChannel, "sink");

  graph.setInstanceImplementation("source-instance", source);
  graph.setInstanceImplementation("sink-instance", lambdaSink);

  graph.startGraph();

  source->sendPacket(Packet(), testChannel);

  usleep(100000);

  ASSERT_EQ(1, receivedPacketCount);
  ASSERT_NE(this_thread::get_id(), packetReceivedOnThreadId);
}

TEST(WhenAPacketIsSentUsingAsyncQueueing, ItArrives) {
  const string testChannel = "test channel";

  DataGraph graph;

  size_t receivedPacketCount = 0;

  thread::id packetReceivedOnThreadId;
  auto source = make_shared<SimpleSource>();
  auto lambdaSink = make_shared<LambdaPathable>(
      [&receivedPacketCount,
       &packetReceivedOnThreadId](const PathablePacket& packet) {
        packetReceivedOnThreadId = this_thread::get_id();
        receivedPacketCount++;
      });

  graph.createNode("source", false, true);
  graph.createNode("sink", true, false);

  graph.setNodeInstance("source", "source-instance");
  graph.setNodeInstance("sink", "sink-instance");

  graph.connect("source", testChannel, "sink", PacketDeliveryType::AlwaysQueue);

  graph.setInstanceImplementation("source-instance", source);
  graph.setInstanceImplementation("sink-instance", lambdaSink);

  graph.startGraph();

  source->sendPacket(Packet(), testChannel);
  usleep(100000);

  // Pushed onto the graph's libuv thread.
  ASSERT_NE(this_thread::get_id(), packetReceivedOnThreadId);
  ASSERT_EQ(1, receivedPacketCount);
}

TEST(WhenAPacketIsSentUsingDirectAndAsyncQueueing, ItArrives) {
  const string testChannel = "test channel";

  DataGraph graph;

  size_t receivedPacketCount = 0;

  thread::id directThreadId;
  thread::id asyncThreadId;
  auto source = make_shared<SimpleSource>();
  auto lambdaSinkDirect = make_shared<LambdaPathable>(
      [&receivedPacketCount, &directThreadId](const PathablePacket& packet) {
        directThreadId = this_thread::get_id();
        cout << "Got direct" << endl;
        receivedPacketCount++;
      });

  auto lambdaSinkAsync = make_shared<LambdaPathable>(
      [&receivedPacketCount, &asyncThreadId](const PathablePacket& packet) {
        asyncThreadId = this_thread::get_id();
        cout << "Got async" << endl;
        receivedPacketCount++;
      });

  graph.createNode("source", false, true);
  graph.createNode("sinkDirect", true, false);
  graph.createNode("sinkAsync", true, false);

  graph.setNodeInstance("source", "source-instance");
  graph.setNodeInstance("sinkDirect", "sink-direct-instance");
  graph.setNodeInstance("sinkAsync", "sink-async-instance");

  graph.connect(
      "source",
      testChannel,
      "sinkDirect",
      PacketDeliveryType::PushDirectlyToTarget);

  graph.connect(
      "source",
      testChannel,
      "sinkAsync",
      PacketDeliveryType::AlwaysQueue);

  graph.setInstanceImplementation("source-instance", source);
  graph.setInstanceImplementation("sink-direct-instance", lambdaSinkDirect);
  graph.setInstanceImplementation("sink-async-instance", lambdaSinkAsync);

  graph.startGraph();

  Packet packet;
  packet.buffers.push_back(Buffer("Tracer"));
  source->sendPacket(packet, testChannel);
  usleep(100000);

  ASSERT_EQ(2, receivedPacketCount);

  // Can't go direct because it's sent from this thread, not the uv thread
  ASSERT_NE(this_thread::get_id(), directThreadId);
  ASSERT_NE(this_thread::get_id(), asyncThreadId);

  // Both dispatched on the default thread group
  ASSERT_EQ(asyncThreadId, directThreadId);
}

TEST(WhenTwoPacketsAreSentToDifferentThreadGroups, TheyHaveDifferentThreadIds) {
  const string testChannel = "test channel";

  DataGraph graph;

  size_t receivedPacketCount = 0;

  thread::id threadId1;
  thread::id threadId2;
  auto source = make_shared<SimpleSource>();
  auto lambdaSink1 = make_shared<LambdaPathable>(
      [&receivedPacketCount, &threadId1](const PathablePacket& packet) {
        threadId1 = this_thread::get_id();
        receivedPacketCount++;
      });

  auto lambdaSink2 = make_shared<LambdaPathable>(
      [&receivedPacketCount, &threadId2](const PathablePacket& packet) {
        threadId2 = this_thread::get_id();
        receivedPacketCount++;
      });

  graph.createNode("source", false, true);
  graph.createNode("sink 1", true, false);
  graph.createNode("sink 2", true, false);

  graph.setNodeInstance("source", "source instance");
  graph.setNodeInstance("sink 1", "sink 1 instance");
  graph.setNodeInstance("sink 2", "sink 2 instance");

  graph.connect(
      "source",
      testChannel,
      "sink 1",
      PacketDeliveryType::AlwaysQueue);

  graph.connect(
      "source",
      testChannel,
      "sink 2",
      PacketDeliveryType::AlwaysQueue);

  graph.setInstanceImplementation("source instance", source);
  graph.setInstanceImplementation("sink 1 instance", lambdaSink1);
  graph.setInstanceImplementation("sink 2 instance", lambdaSink2);

  graph.setThreadGroupForInstance("sink 2 instance", "test");

  graph.startGraph();

  source->sendPacket(Packet(), testChannel);
  usleep(100000);

  ASSERT_NE(this_thread::get_id(), threadId1);
  ASSERT_NE(this_thread::get_id(), threadId2);

  ASSERT_NE(threadId1, threadId2);

  ASSERT_EQ(2, receivedPacketCount);
}

TEST(
    WhenTwoPacketsAreSentToNonDefaultDifferentThreadGroups,
    TheyHaveDifferentThreadIds) {
  const string testChannel = "test channel";

  DataGraph graph;

  size_t receivedPacketCount = 0;

  thread::id threadId1;
  thread::id threadId2;
  auto source = make_shared<SimpleSource>();
  auto lambdaSink1 = make_shared<LambdaPathable>(
      [&receivedPacketCount, &threadId1](const PathablePacket& packet) {
        threadId1 = this_thread::get_id();
        receivedPacketCount++;
      });

  auto lambdaSink2 = make_shared<LambdaPathable>(
      [&receivedPacketCount, &threadId2](const PathablePacket& packet) {
        threadId2 = this_thread::get_id();
        receivedPacketCount++;
      });

  graph.createNode("source", false, true);
  graph.createNode("sink 1", true, false);
  graph.createNode("sink 2", true, false);

  graph.setNodeInstance("source", "source instance");
  graph.setNodeInstance("sink 1", "sink 1 instance");
  graph.setNodeInstance("sink 2", "sink 2 instance");

  graph.connect(
      "source",
      testChannel,
      "sink 1",
      PacketDeliveryType::AlwaysQueue);

  graph.connect(
      "source",
      testChannel,
      "sink 2",
      PacketDeliveryType::AlwaysQueue);

  graph.setInstanceImplementation("source instance", source);
  graph.setInstanceImplementation("sink 1 instance", lambdaSink1);
  graph.setInstanceImplementation("sink 2 instance", lambdaSink2);

  graph.setThreadGroupForInstance("sink 1 instance", "test 1");
  graph.setThreadGroupForInstance("sink 2 instance", "test 2");

  graph.startGraph();

  source->sendPacket(Packet(), testChannel);
  usleep(100000);

  ASSERT_NE(this_thread::get_id(), threadId1);
  ASSERT_NE(this_thread::get_id(), threadId2);

  ASSERT_NE(threadId1, threadId2);

  ASSERT_EQ(2, receivedPacketCount);
}

TEST(WhenOneSourceSendsAPacketAndTwoSinksReceive, ThePacketIsTheSame) {
  DataGraph graph;

  Packet packet;
  packet.parameters["key1"] = "value1";

  size_t receivedPacketCount = 0;

  thread::id directThreadId;
  thread::id asyncThreadId;
  auto source = make_shared<SimpleSource>();
  auto lambdaSinkDirect =
      make_shared<LambdaPathable>([&receivedPacketCount, &directThreadId](
                                      const PathablePacket& pathablePacket) {
        const auto& packet = pathablePacket.packet;

        directThreadId = this_thread::get_id();
        receivedPacketCount++;

        ASSERT_TRUE(packet.parameters.contains("key1"))
            << packet.parameters.dump(2);
        ASSERT_EQ("value1", packet.parameters["key1"].get<string>());
      });

  auto lambdaSinkAsync =
      make_shared<LambdaPathable>([&receivedPacketCount, &asyncThreadId](
                                      const PathablePacket& pathablePacket) {
        const auto& packet = pathablePacket.packet;

        asyncThreadId = this_thread::get_id();
        receivedPacketCount++;

        ASSERT_TRUE(packet.parameters.contains("key1"))
            << packet.parameters.dump(2);
        ASSERT_EQ("value1", packet.parameters["key1"].get<string>());
      });

  graph.createNode("source", false, true);
  graph.createNode("pass-through", true, true);
  graph.createNode("sink direct", true, false);
  graph.createNode("sink async", true, false);

  graph.setNodeInstance("source", "source instance");
  graph.setNodeInstance("pass-through", "pass-through instance");
  graph.setNodeInstance("sink direct", "sink direct instance");
  graph.setNodeInstance("sink async", "sink async instance");

  graph.connect("source", "test channel", "pass-through");
  graph.connect(
      "pass-through",
      "test channel",
      "sink direct",
      PacketDeliveryType::PushDirectlyToTarget);

  graph.connect(
      "pass-through",
      "test channel",
      "sink async",
      PacketDeliveryType::AlwaysQueue);

  graph.setInstanceInitParameters("pass-through instance", R"({
        "outputChannel": "test channel"
      })"_json);

  graph.setInstanceImplementation("source instance", source);
  graph.setInstanceType("pass-through instance", "Pass-through");
  graph.setInstanceImplementation("sink direct instance", lambdaSinkDirect);
  graph.setInstanceImplementation("sink async instance", lambdaSinkAsync);

  graph.startGraph();

  source->sendPacket(packet, "test channel");
  usleep(100000);

  ASSERT_EQ(2, receivedPacketCount);

  // Can't go direct because it's sent from this thread, not the uv thread
  ASSERT_NE(this_thread::get_id(), directThreadId);
  ASSERT_NE(this_thread::get_id(), asyncThreadId);

  // Both dispatched on the default thread group
  ASSERT_EQ(asyncThreadId, directThreadId);
}

}  // namespace maplang
