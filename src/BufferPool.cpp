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

#include "maplang/BufferPool.h"

#include <stack>

#include "concurrentqueue.h"

using namespace std;

namespace maplang {

struct BufferPool::Impl final {
  using QueueType = moodycamel::ConcurrentQueue<shared_ptr<uint8_t>>;

  Impl() : bufferSize(0) {}

  Allocator allocator;
  shared_ptr<QueueType> bufferQueue;
  size_t bufferSize;
};

BufferPool::BufferPool() : mImpl(make_shared<BufferPool::Impl>()) {}

void BufferPool::setAllocator(Allocator&& allocator) {
  mImpl->bufferQueue.reset();
  mImpl->bufferSize = 0;
  mImpl->allocator = allocator;
}

void BufferPool::setAllocator(const Allocator& allocator) {
  mImpl->bufferQueue.reset();
  mImpl->bufferSize = 0;
  mImpl->allocator = allocator;
}

shared_ptr<uint8_t> BufferPool::get(size_t bufferSize) {
  if (bufferSize == 0) {
    return shared_ptr<uint8_t>(nullptr);
  }

  if (bufferSize > mImpl->bufferSize) {
    mImpl->bufferQueue.reset();
    mImpl->bufferQueue = make_shared<Impl::QueueType>();
    mImpl->bufferSize = bufferSize;
  }

  shared_ptr<uint8_t> sourceBuffer;
  if (!mImpl->bufferQueue->try_dequeue(sourceBuffer)) {
    if (mImpl->allocator == nullptr) {
      throw runtime_error(
          "BufferPool Allocator was not set before requesting frames.");
    }

    sourceBuffer = mImpl->allocator(bufferSize);
  }

  const weak_ptr<Impl::QueueType> weakBufferQueue = mImpl->bufferQueue;
  return shared_ptr<uint8_t>(
      sourceBuffer.get(),
      [weakBufferQueue, sourceBuffer](uint8_t* rawBuffer) {
        const auto bufferQueue = weakBufferQueue.lock();

        if (bufferQueue == nullptr) {
          return;
        }

        bufferQueue->enqueue(sourceBuffer);
      });
}

}  // namespace maplang
