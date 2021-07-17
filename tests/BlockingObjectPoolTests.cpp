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
#include "maplang/BlockingObjectPool.h"

using namespace maplang;
using namespace std;

static BlockingObjectPool<string>::Allocator createAllocator() {
  return []() { return make_shared<string>("hello"); };
}

TEST(WhenAStringIsRequested, ItReturnsOne) {
  BlockingObjectPool<string> pool(1);
  pool.setAllocator(createAllocator());

  const auto str = pool.get();

  ASSERT_NE(nullptr, str);
  ASSERT_STREQ(str->c_str(), "hello");
}

TEST(WhenTwoStringsAreRequestedFromAPoolOfSize1, ItReturnsTheSameOne) {
  BlockingObjectPool<string> pool(1);
  pool.setAllocator(createAllocator());

  auto str = pool.get();

  void* origStrAddress = str.get();
  ASSERT_NE(nullptr, str);
  ASSERT_STREQ(str->c_str(), "hello");

  thread t([&str]() { str.reset(); });
  t.detach();

  const auto str2 = pool.get();
  ASSERT_NE(nullptr, str2);
  ASSERT_STREQ(str2->c_str(), "hello");
  ASSERT_EQ(origStrAddress, str2.get());
}
