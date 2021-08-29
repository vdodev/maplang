/*
 *Copyright 2020 VDO Dev Inc <support@maplang.com>
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

#include "gtest/gtest.h"
#include "maplang/DataGraph.h"
#include "maplang/FactoriesBuilder.h"
#include "maplang/GraphBuilder.h"
#include "maplang/LambdaPacketPusher.h"
#include "maplang/LambdaPathable.h"
#include "maplang/SimpleSource.h"

using namespace std;

namespace maplang {

class BufferAccumulatorNodeTests : public testing::Test {
 public:
  BufferAccumulatorNodeTests()
      : mFactories(FactoriesBuilder().BuildFactories()) {}

  const Factories mFactories;
};

static const string dotGraph = R"(
    digraph BufferAccumulatorTest {
      compound=true

      "Test Input" [instance="Test Input Instance", allowOutgoing=true]

      "Test Output" [
        instance="Test Output Instance",
        allowIncoming=true,
        allowOutgoing=true]

      subgraph "Cluster Buffer Accumulator" {
        instance="Buffer Accumulator Instance"
        label="Buffer Accumulator"

        "Append Buffers" [
          instance="Buffer Accumulator Append Instance",
          allowIncoming=true]

        "Send Accumulated Buffers" [
          instance="Buffer Accumulator Send Instance",
          allowIncoming=true,
          allowOutgoing=true]

        "Clear Buffers" [
          instance="Buffer Accumulator Clear Instance",
          allowIncoming=true]
      }

      "Test Input" -> "Append Buffers" [label="New Buffer"]
      "Test Input" -> "Send Accumulated Buffers" [label="Send Buffers"]

      "Send Accumulated Buffers" -> "Test Output" [label="Buffers Ready"]
    }
  )";

static const string implementation = R"({
    "Buffer Accumulator Instance": {
      "type": "Buffer Accumulator",
      "instanceToInterfaceMap": {
        "Buffer Accumulator Append Instance": {
          "interface": "Append Buffers"
        },
        "Buffer Accumulator Send Instance": {
          "interface": "Send Accumulated Buffers"
        },
        "Buffer Accumulator Clear Instance": {
          "interface": "Clear Buffers"
        }
      }
    }
  })";

TEST_F(BufferAccumulatorNodeTests, WhenASingleBufferIsSent_ItIsReceived) {
  const auto graph = buildDataGraph(mFactories, dotGraph);
  implementDataGraph(graph, implementation);

  const auto testInput = make_shared<SimpleSource>();
  graph->setInstanceImplementation("Test Input Instance", testInput);

  Buffer receivedBuffer;
  size_t receivedBufferCount = 0;
  size_t receivedPacketCount = 0;
  const auto testOutput = make_shared<LambdaPathable>(
      [&receivedBuffer, &receivedBufferCount, &receivedPacketCount](
          const PathablePacket& packet) {
        receivedBufferCount = packet.packet.buffers.size();

        if (receivedBufferCount > 0) {
          receivedBuffer = packet.packet.buffers[0];
        }

        receivedPacketCount++;
      });

  graph->setInstanceImplementation("Test Output Instance", testOutput);
  graph->startGraph();

  Packet packet;
  packet.buffers.push_back(Buffer("test"));
  testInput->sendPacket(packet, "New Buffer");

  testInput->sendPacket(Packet(), "Send Buffers");

  this_thread::sleep_for(chrono::milliseconds(100));

  EXPECT_EQ(1, receivedPacketCount);
  EXPECT_EQ(1, receivedBufferCount);

  EXPECT_EQ(strlen("test"), receivedBuffer.length);

  EXPECT_EQ(
      0,
      memcmp("test", receivedBuffer.data.get(), receivedBuffer.length));
}

TEST_F(
    BufferAccumulatorNodeTests,
    WhenTwoBuffersAreSent_TheOutputBufferIsCorrect) {
  const auto graph = buildDataGraph(mFactories, dotGraph);
  implementDataGraph(graph, implementation);

  const auto testInput = make_shared<SimpleSource>();
  graph->setInstanceImplementation("Test Input Instance", testInput);

  Buffer receivedBuffer;
  size_t receivedBufferCount = 0;
  size_t receivedPacketCount = 0;
  const auto testOutput = make_shared<LambdaPathable>(
      [&receivedBuffer, &receivedBufferCount, &receivedPacketCount](
          const PathablePacket& packet) {
        receivedBufferCount = packet.packet.buffers.size();

        if (receivedBufferCount > 0) {
          receivedBuffer = packet.packet.buffers[0];
        }

        receivedPacketCount++;
      });

  graph->setInstanceImplementation("Test Output Instance", testOutput);
  graph->startGraph();

  Packet packet;
  packet.buffers.push_back(Buffer("test"));
  testInput->sendPacket(packet, "New Buffer");

  Packet packet2;
  packet2.buffers.push_back(Buffer(", hello"));
  testInput->sendPacket(packet2, "New Buffer");

  testInput->sendPacket(Packet(), "Send Buffers");

  this_thread::sleep_for(chrono::milliseconds(100));

  EXPECT_EQ(1, receivedPacketCount);
  EXPECT_EQ(1, receivedBufferCount);

  EXPECT_EQ(strlen("test, hello"), receivedBuffer.length);

  EXPECT_EQ(
      0,
      memcmp("test, hello", receivedBuffer.data.get(), receivedBuffer.length));
}

TEST_F(
    BufferAccumulatorNodeTests,
    WhenMultiplePacketsWithMultipleBuffersAreSent_TheOutputBuffersAreCorrect) {
  const auto graph = buildDataGraph(mFactories, dotGraph);
  implementDataGraph(graph, implementation);

  const auto testInput = make_shared<SimpleSource>();
  graph->setInstanceImplementation("Test Input Instance", testInput);

  Buffer receivedBuffer;
  Buffer receivedBuffer2;
  size_t receivedBufferCount = 0;
  size_t receivedPacketCount = 0;
  const auto testOutput = make_shared<LambdaPathable>(
      [&receivedBuffer,
       &receivedBuffer2,
       &receivedBufferCount,
       &receivedPacketCount](const PathablePacket& packet) {
        receivedBufferCount = packet.packet.buffers.size();

        if (receivedBufferCount > 0) {
          receivedBuffer = packet.packet.buffers[0];
        }

        if (receivedBufferCount > 1) {
          receivedBuffer2 = packet.packet.buffers[1];
        }

        receivedPacketCount++;
      });

  graph->setInstanceImplementation("Test Output Instance", testOutput);
  graph->startGraph();

  Packet packet;
  packet.buffers.push_back(Buffer("test"));
  testInput->sendPacket(packet, "New Buffer");

  Packet packet2;
  packet2.buffers.push_back(Buffer(", hello"));
  testInput->sendPacket(packet2, "New Buffer");

  Packet packet3;
  packet3.buffers.push_back(Buffer(", packet3"));
  packet3.buffers.push_back(Buffer("second buffer"));
  testInput->sendPacket(packet3, "New Buffer");

  testInput->sendPacket(Packet(), "Send Buffers");

  this_thread::sleep_for(chrono::milliseconds(100));

  EXPECT_EQ(1, receivedPacketCount);
  EXPECT_EQ(2, receivedBufferCount);

  EXPECT_EQ(strlen("test, hello, packet3"), receivedBuffer.length);
  EXPECT_EQ(
      0,
      memcmp(
          "test, hello, packet3",
          receivedBuffer.data.get(),
          receivedBuffer.length));

  EXPECT_EQ(strlen("second buffer"), receivedBuffer2.length);
  EXPECT_EQ(
      0,
      memcmp(
          "second buffer",
          receivedBuffer2.data.get(),
          receivedBuffer2.length));
}

}  // namespace maplang
