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
#include "nodes/SendOnce.h"

using namespace std;

namespace maplang {

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

  DataGraph graph(uvLoopRunner.getLoop());

  Packet packet;
  size_t receivedPacketCount = 0;

  auto sendOnce = make_shared<SendOnce>(nlohmann::json());
  thread::id packetReceivedOnThreadId;

  auto lambdaSink = make_shared<LambdaSink>(
      [&receivedPacketCount, &packetReceivedOnThreadId](const Packet& packet) {
        packetReceivedOnThreadId = this_thread::get_id();
        receivedPacketCount++;
      });

  graph.connect(sendOnce, "initialized", lambdaSink);

  uv_stop(uvLoopRunner.getLoop().get());
  uvLoopRunner.waitForExit();

  ASSERT_EQ(this_thread::get_id(), packetReceivedOnThreadId);
  ASSERT_EQ(1, receivedPacketCount);
}

TEST(WhenAPacketIsSentUsingAsyncQueueing, ItArrives) {
  UvLoopRunner uvLoopRunner;

  DataGraph graph(uvLoopRunner.getLoop());

  Packet packet;
  size_t receivedPacketCount = 0;

  thread::id packetReceivedOnThreadId;
  auto sendOnce = make_shared<SendOnce>(nlohmann::json());
  auto lambdaSink = make_shared<LambdaSink>(
      [&receivedPacketCount, &packetReceivedOnThreadId](const Packet& packet) {
        packetReceivedOnThreadId = this_thread::get_id();
        receivedPacketCount++;
      });

  graph.connect(
      sendOnce,
      "initialized",
      lambdaSink,
      "",
      "",
      PacketDeliveryType::AlwaysQueue);

  usleep(1000);

  ASSERT_EQ(this_thread::get_id(), packetReceivedOnThreadId);
  uv_stop(uvLoopRunner.getLoop().get());
  uvLoopRunner.waitForExit();

  ASSERT_EQ(1, receivedPacketCount);
}

}  // namespace maplang
