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
    return shared_ptr<uint8_t>(new uint8_t[size], default_delete<uint8_t[]>());
  };
}

TEST(WhenTheFirstBufferIsRequestedWithAMovableAllocator, ItReturnsABuffer) {
  BufferPool pool;
  pool.setAllocator(createAllocator());

  const auto buffer = pool.get(1);

  ASSERT_NE(nullptr, buffer);
}

TEST(WhenTheFirstBufferIsRequestedWithAConstReferenceAllocator, ItReturnsABuffer) {
  BufferPool pool;
  const auto allocator = createAllocator();
  pool.setAllocator(allocator);

  const auto buffer = pool.get(1);

  ASSERT_NE(nullptr, buffer);
}

TEST(WhenThePoolIsDeallocatedBeforeABuffer, ItDoesntCrash) {
  shared_ptr<uint8_t> buffer;

  {
    BufferPool pool;
    pool.setAllocator(createAllocator());

    buffer = pool.get(1);
  }
}

TEST(WhenASecondBufferIsRequestedBeforeTheFirstBufferIsReturned, ItReturnsADifferentBuffer) {
  BufferPool pool;
  pool.setAllocator(createAllocator());

  const auto buffer1 = pool.get(1);
  const auto buffer2 = pool.get(1);

  ASSERT_NE(nullptr, buffer1);
  ASSERT_NE(nullptr, buffer2);
  ASSERT_NE(buffer1, buffer2);
}

TEST(WhenASecondBufferIsRequestedAfterTheFirstBufferIsReturned, ItReturnsTheSameBuffer) {
  BufferPool pool;
  pool.setAllocator(createAllocator());

  auto buffer1 = pool.get(1);
  uint8_t *const rawBuffer1 = buffer1.get();
  buffer1.reset();

  const auto buffer2 = pool.get(1);

  ASSERT_NE(nullptr, rawBuffer1);
  ASSERT_NE(nullptr, buffer2);
  ASSERT_EQ(rawBuffer1, buffer2.get());
}

TEST(WhenABufferIsRequestedAndTheAllocatorIsNotSet, ItThrowsAnException) {
  BufferPool pool;
  ASSERT_ANY_THROW(pool.get(1));
}

TEST(WhenAnEmptyBufferIsRequested, ItReturnsAnEmptyBuffer) {
  BufferPool pool;
  pool.setAllocator(createAllocator());

  const auto buffer = pool.get(0);
  ASSERT_EQ(nullptr, buffer);
}

TEST(WhenAnEmptyBufferIsRequestedAndTheAllocatorIsNotSet, ItReturnsAnEmptyBuffer) {
  BufferPool pool;
  const auto buffer = pool.get(0);
  ASSERT_EQ(nullptr, buffer);
}

}  // namespace maplang
