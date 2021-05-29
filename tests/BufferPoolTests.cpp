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
#include "maplang/BufferPool.h"

using namespace std;

namespace maplang {

static BufferPool::Allocator createAllocator() {
  return [](size_t size) {
    Buffer buffer;

    buffer.data = shared_ptr<uint8_t[]>(new uint8_t[size]);
    buffer.length = size;

    return buffer;
  };
}

TEST(WhenTheFirstBufferIsRequestedWithAMovableAllocator, ItReturnsABuffer) {
  BufferPool pool;
  pool.setAllocator(createAllocator());

  const auto buffer = pool.get(1);

  ASSERT_NE(nullptr, buffer.data);
}

TEST(
    WhenTheFirstBufferIsRequestedWithAConstReferenceAllocator,
    ItReturnsABuffer) {
  BufferPool pool;
  const auto allocator = createAllocator();
  pool.setAllocator(allocator);

  const auto buffer = pool.get(1);

  ASSERT_NE(nullptr, buffer.data);
}

TEST(WhenThePoolIsDeallocatedBeforeABuffer, ItDoesntCrash) {
  Buffer buffer;

  {
    BufferPool pool;
    pool.setAllocator(createAllocator());

    buffer = pool.get(1);
  }
}

TEST(
    WhenASecondBufferIsRequestedBeforeTheFirstBufferIsReturned,
    ItReturnsADifferentBuffer) {
  BufferPool pool;
  pool.setAllocator(createAllocator());

  const auto buffer1 = pool.get(1);
  const auto buffer2 = pool.get(1);

  ASSERT_NE(nullptr, buffer1.data);
  ASSERT_NE(nullptr, buffer2.data);
  ASSERT_NE(buffer1.data, buffer2.data);
}

TEST(
    WhenASecondBufferIsRequestedAfterTheFirstBufferIsReturned,
    ItReturnsTheSameBuffer) {
  BufferPool pool;
  pool.setAllocator(createAllocator());

  auto buffer1 = pool.get(1);
  uint8_t* const rawBuffer1 = buffer1.data.get();
  buffer1.data.reset();

  const auto buffer2 = pool.get(1);

  ASSERT_NE(nullptr, rawBuffer1);
  ASSERT_NE(nullptr, buffer2.data);
  ASSERT_EQ(rawBuffer1, buffer2.data.get());
}

TEST(WhenABufferIsRequestedAndTheAllocatorIsNotSet, ItThrowsAnException) {
  BufferPool pool;
  ASSERT_ANY_THROW(pool.get(1));
}

TEST(WhenAnEmptyBufferIsRequested, ItReturnsAnEmptyBuffer) {
  BufferPool pool;
  pool.setAllocator(createAllocator());

  const auto buffer = pool.get(0);
  ASSERT_EQ(nullptr, buffer.data);
  ASSERT_EQ(0, buffer.length);
}

TEST(
    WhenAnEmptyBufferIsRequestedAndTheAllocatorIsNotSet,
    ItReturnsAnEmptyBuffer) {
  BufferPool pool;
  const auto buffer = pool.get(0);
  ASSERT_EQ(nullptr, buffer.data);
  ASSERT_EQ(0, buffer.length);
}

TEST(WhenALargerBufferIsRequested, ALargerBufferIsReturned) {
  BufferPool pool;
  pool.setAllocator(createAllocator());

  auto buffer1 = pool.get(1);
  auto buffer2 = pool.get(1);
  buffer1.data.reset();

  auto buffer3 = pool.get(2);
  ASSERT_NE(nullptr, buffer3.data);
  ASSERT_EQ(2, buffer3.length);
}

TEST(
    WhenALargerBufferThanRequestedIsReturned,
    ItIsRecycledUsingItsOriginalSize) {
  BufferPool pool;
  pool.setAllocator(createAllocator());

  auto buffer1 = pool.get(2);
  auto buffer2 = pool.get(1);
  buffer2.data.reset();

  auto buffer3 = pool.get(2);
  ASSERT_NE(nullptr, buffer3.data);
  ASSERT_EQ(2, buffer3.length);
}

}  // namespace maplang
