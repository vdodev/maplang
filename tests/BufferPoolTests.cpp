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
    return new uint8_t[size];
  };
}

static BufferPool::Deallocator createDeallocator() {
  return [](uint8_t* buffer) {
    delete[] buffer;
  };
}

TEST(WhenTheFirstBufferIsRequested, ItReturnsABuffer) {
  BufferPool pool;
  pool.setAllocator(createAllocator(), createDeallocator());

  const auto buffer = pool.get(1);

  ASSERT_NE(nullptr, buffer);
}

TEST(WhenASecondBufferIsRequestedBeforeTheFirstBufferIsReturned, ItReturnsADifferentBuffer) {
  BufferPool pool;
  pool.setAllocator(createAllocator(), createDeallocator());

  const auto buffer1 = pool.get(1);
  const auto buffer2 = pool.get(1);

  ASSERT_NE(nullptr, buffer1);
  ASSERT_NE(nullptr, buffer2);
  ASSERT_NE(buffer1, buffer2);
}

TEST(WhenASecondBufferIsRequestedAfterTheFirstBufferIsReturned, ItReturnsTheSameBuffer) {
  BufferPool pool;
  pool.setAllocator(createAllocator(), createDeallocator());

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

}  // namespace maplang
