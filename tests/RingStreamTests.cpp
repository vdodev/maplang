/*
 * Copyright 2020 VDO Dev Inc <support@maplang.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "gtest/gtest.h"
#include "maplang/FactoriesBuilder.h"
#include "maplang/RingStream.h"

using namespace std;
using namespace maplang;

class RingStreamTests : public testing::Test {
 public:
  RingStreamTests() : mFactories(FactoriesBuilder().BuildFactories()) {}

  const Factories mFactories;
};

TEST_F(RingStreamTests, WhenABufferIsWrittenThenRead_ItMatches) {
  RingStream ringStream(mFactories.bufferFactory);

  uint8_t buffer[] = {1, 2, 3, 4, 5};
  ringStream.Write(buffer, sizeof(buffer));

  uint8_t outputBuffer[5];
  ringStream.Read(outputBuffer, sizeof(outputBuffer));

  EXPECT_EQ(1, outputBuffer[0]);
  EXPECT_EQ(2, outputBuffer[1]);
  EXPECT_EQ(3, outputBuffer[2]);
  EXPECT_EQ(4, outputBuffer[3]);
  EXPECT_EQ(5, outputBuffer[4]);
}

TEST_F(RingStreamTests, WhenTwoBufferAreWrittenThenRead_ItMatches) {
  RingStream ringStream(mFactories.bufferFactory);

  uint8_t buffer1[] = {1, 2, 3, 4, 5};
  uint8_t buffer2[] = {6, 7, 8, 9, 0};
  ringStream.Write(buffer1, sizeof(buffer1));
  ringStream.Write(buffer2, sizeof(buffer2));

  uint8_t outputBuffer[sizeof(buffer1) + sizeof(buffer2)];
  ringStream.Read(outputBuffer, sizeof(outputBuffer));

  EXPECT_EQ(1, outputBuffer[0]);
  EXPECT_EQ(2, outputBuffer[1]);
  EXPECT_EQ(3, outputBuffer[2]);
  EXPECT_EQ(4, outputBuffer[3]);
  EXPECT_EQ(5, outputBuffer[4]);
  EXPECT_EQ(6, outputBuffer[5]);
  EXPECT_EQ(7, outputBuffer[6]);
  EXPECT_EQ(8, outputBuffer[7]);
  EXPECT_EQ(9, outputBuffer[8]);
  EXPECT_EQ(0, outputBuffer[9]);
}

TEST_F(RingStreamTests, WhenAWriteWrapsTheInternalBuffer_ItIsReadyCorrectly) {
  static constexpr size_t kInitialBufferSize = 5;
  RingStream ringStream(mFactories.bufferFactory, kInitialBufferSize);

  uint8_t buffer1[] = {1, 2};
  uint8_t buffer2[] = {3, 4, 5, 6};
  ringStream.Write(buffer1, sizeof(buffer1));

  uint8_t firstOutputByte = 0;
  ringStream.Read(&firstOutputByte, 1);
  ringStream.Write(buffer2, sizeof(buffer2));

  EXPECT_EQ(kInitialBufferSize, ringStream.GetCapacity());
  EXPECT_EQ(5, ringStream.GetAvailableByteCount());

  uint8_t outputBuffer[5];
  const size_t readByteCount = ringStream.Read(outputBuffer, sizeof(outputBuffer));

  EXPECT_EQ(sizeof(outputBuffer), readByteCount);
  EXPECT_EQ(2, outputBuffer[0]);
  EXPECT_EQ(3, outputBuffer[1]);
  EXPECT_EQ(4, outputBuffer[2]);
  EXPECT_EQ(5, outputBuffer[3]);
  EXPECT_EQ(6, outputBuffer[4]);
}
