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

class DataGraphTests : public testing::Test {
 public:
  DataGraphTests()
      : mDataGraph(
          make_shared<DataGraph>(FactoriesBuilder().BuildFactories())) {}

  const std::shared_ptr<DataGraph> mDataGraph;
};

TEST_F(
    DataGraphTests,
    WhenSendPacketIsCalledOnce_ThenOnePacketIsDeliveredToTheSink) {
  size_t receivedPacketCount = 0;
  auto lambdaSink = make_shared<LambdaPathable>(
      [&receivedPacketCount](const PathablePacket& packet) {
        receivedPacketCount++;
      });

  mDataGraph->createNode("sink", false, true);
  mDataGraph->setNodeInstance("sink", "sink-instance");
  mDataGraph->setInstanceImplementation("sink-instance", lambdaSink);

  mDataGraph->sendPacket(Packet(), "sink");

  usleep(100000);

  ASSERT_EQ(1, receivedPacketCount);
}

TEST_F(
    DataGraphTests,
    WhenAPacketIsSentDirectlyFromADifferentThread_ItArrives) {
  const string testChannel = "test channel";

  size_t receivedPacketCount = 0;

  auto source = make_shared<SimpleSource>();
  thread::id packetReceivedOnThreadId;

  auto lambdaSink = make_shared<LambdaPathable>(
      [&receivedPacketCount,
       &packetReceivedOnThreadId](const PathablePacket& packet) {
        packetReceivedOnThreadId = this_thread::get_id();
        receivedPacketCount++;
      });

  mDataGraph->createNode("source", false, true);
  mDataGraph->createNode("sink", true, false);

  mDataGraph->setNodeInstance("source", "source-instance");
  mDataGraph->setNodeInstance("sink", "sink-instance");

  mDataGraph->connect("source", testChannel, "sink");

  mDataGraph->setInstanceImplementation("source-instance", source);
  mDataGraph->setInstanceImplementation("sink-instance", lambdaSink);

  mDataGraph->startGraph();

  source->sendPacket(Packet(), testChannel);

  usleep(100000);

  ASSERT_EQ(1, receivedPacketCount);
  ASSERT_NE(this_thread::get_id(), packetReceivedOnThreadId);
}

TEST_F(DataGraphTests, WhenAPacketIsSentUsingAsyncQueueing_ItArrives) {
  const string testChannel = "test channel";

  size_t receivedPacketCount = 0;

  thread::id packetReceivedOnThreadId;
  auto source = make_shared<SimpleSource>();
  auto lambdaSink = make_shared<LambdaPathable>(
      [&receivedPacketCount,
       &packetReceivedOnThreadId](const PathablePacket& packet) {
        packetReceivedOnThreadId = this_thread::get_id();
        receivedPacketCount++;
      });

  mDataGraph->createNode("source", false, true);
  mDataGraph->createNode("sink", true, false);

  mDataGraph->setNodeInstance("source", "source-instance");
  mDataGraph->setNodeInstance("sink", "sink-instance");

  mDataGraph
      ->connect("source", testChannel, "sink", PacketDeliveryType::AlwaysQueue);

  mDataGraph->setInstanceImplementation("source-instance", source);
  mDataGraph->setInstanceImplementation("sink-instance", lambdaSink);

  mDataGraph->startGraph();

  source->sendPacket(Packet(), testChannel);
  usleep(100000);

  // Pushed onto the graph's libuv thread.
  ASSERT_NE(this_thread::get_id(), packetReceivedOnThreadId);
  ASSERT_EQ(1, receivedPacketCount);
}

TEST_F(DataGraphTests, WhenAPacketIsSentUsingDirectAndAsyncQueueing_ItArrives) {
  const string testChannel = "test channel";

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

  mDataGraph->createNode("source", false, true);
  mDataGraph->createNode("sinkDirect", true, false);
  mDataGraph->createNode("sinkAsync", true, false);

  mDataGraph->setNodeInstance("source", "source-instance");
  mDataGraph->setNodeInstance("sinkDirect", "sink-direct-instance");
  mDataGraph->setNodeInstance("sinkAsync", "sink-async-instance");

  mDataGraph->connect(
      "source",
      testChannel,
      "sinkDirect",
      PacketDeliveryType::PushDirectlyToTarget);

  mDataGraph->connect(
      "source",
      testChannel,
      "sinkAsync",
      PacketDeliveryType::AlwaysQueue);

  mDataGraph->setInstanceImplementation("source-instance", source);
  mDataGraph->setInstanceImplementation(
      "sink-direct-instance",
      lambdaSinkDirect);
  mDataGraph->setInstanceImplementation("sink-async-instance", lambdaSinkAsync);

  mDataGraph->startGraph();

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

TEST_F(
    DataGraphTests,
    WhenTwoPacketsAreSentToDifferentThreadGroups_TheyHaveDifferentThreadIds) {
  const string testChannel = "test channel";
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

  mDataGraph->createNode("source", false, true);
  mDataGraph->createNode("sink 1", true, false);
  mDataGraph->createNode("sink 2", true, false);

  mDataGraph->setNodeInstance("source", "source instance");
  mDataGraph->setNodeInstance("sink 1", "sink 1 instance");
  mDataGraph->setNodeInstance("sink 2", "sink 2 instance");

  mDataGraph->connect(
      "source",
      testChannel,
      "sink 1",
      PacketDeliveryType::AlwaysQueue);

  mDataGraph->connect(
      "source",
      testChannel,
      "sink 2",
      PacketDeliveryType::AlwaysQueue);

  mDataGraph->setInstanceImplementation("source instance", source);
  mDataGraph->setInstanceImplementation("sink 1 instance", lambdaSink1);
  mDataGraph->setInstanceImplementation("sink 2 instance", lambdaSink2);

  mDataGraph->setThreadGroupForInstance("sink 2 instance", "test");

  mDataGraph->startGraph();

  source->sendPacket(Packet(), testChannel);
  usleep(100000);

  ASSERT_NE(this_thread::get_id(), threadId1);
  ASSERT_NE(this_thread::get_id(), threadId2);

  ASSERT_NE(threadId1, threadId2);

  ASSERT_EQ(2, receivedPacketCount);
}

TEST_F(
    DataGraphTests,
    WhenTwoPacketsAreSentToNonDefaultDifferentThreadGroups_TheyHaveDifferentThreadIds) {
  const string testChannel = "test channel";

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

  mDataGraph->createNode("source", false, true);
  mDataGraph->createNode("sink 1", true, false);
  mDataGraph->createNode("sink 2", true, false);

  mDataGraph->setNodeInstance("source", "source instance");
  mDataGraph->setNodeInstance("sink 1", "sink 1 instance");
  mDataGraph->setNodeInstance("sink 2", "sink 2 instance");

  mDataGraph->connect(
      "source",
      testChannel,
      "sink 1",
      PacketDeliveryType::AlwaysQueue);

  mDataGraph->connect(
      "source",
      testChannel,
      "sink 2",
      PacketDeliveryType::AlwaysQueue);

  mDataGraph->setInstanceImplementation("source instance", source);
  mDataGraph->setInstanceImplementation("sink 1 instance", lambdaSink1);
  mDataGraph->setInstanceImplementation("sink 2 instance", lambdaSink2);

  mDataGraph->setThreadGroupForInstance("sink 1 instance", "test 1");
  mDataGraph->setThreadGroupForInstance("sink 2 instance", "test 2");

  mDataGraph->startGraph();

  source->sendPacket(Packet(), testChannel);
  usleep(100000);

  ASSERT_NE(this_thread::get_id(), threadId1);
  ASSERT_NE(this_thread::get_id(), threadId2);

  ASSERT_NE(threadId1, threadId2);

  ASSERT_EQ(2, receivedPacketCount);
}

TEST_F(
    DataGraphTests,
    WhenOneSourceSendsAPacketAndTwoSinksReceive_ThePacketIsTheSame) {
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

  mDataGraph->createNode("source", false, true);
  mDataGraph->createNode("pass-through", true, true);
  mDataGraph->createNode("sink direct", true, false);
  mDataGraph->createNode("sink async", true, false);

  mDataGraph->setNodeInstance("source", "source instance");
  mDataGraph->setNodeInstance("pass-through", "pass-through instance");
  mDataGraph->setNodeInstance("sink direct", "sink direct instance");
  mDataGraph->setNodeInstance("sink async", "sink async instance");

  mDataGraph->connect("source", "test channel", "pass-through");
  mDataGraph->connect(
      "pass-through",
      "test channel",
      "sink direct",
      PacketDeliveryType::PushDirectlyToTarget);

  mDataGraph->connect(
      "pass-through",
      "test channel",
      "sink async",
      PacketDeliveryType::AlwaysQueue);

  mDataGraph->setInstanceInitParameters("pass-through instance", R"({
        "outputChannel": "test channel"
      })"_json);

  mDataGraph->setInstanceImplementation("source instance", source);
  mDataGraph->setInstanceType("pass-through instance", "Pass-through");
  mDataGraph->setInstanceImplementation(
      "sink direct instance",
      lambdaSinkDirect);
  mDataGraph->setInstanceImplementation("sink async instance", lambdaSinkAsync);

  mDataGraph->startGraph();

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
