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

#include "maplang/concurrentqueue.h"

using namespace std;

namespace maplang {

struct BufferPool::Impl final {
  using QueueType = moodycamel::ConcurrentQueue<Buffer>;

  Impl(const std::shared_ptr<IBufferFactory>& _bufferFactory)
      : bufferFactory(_bufferFactory), bufferSize(0) {}

  const std::shared_ptr<IBufferFactory> bufferFactory;
  shared_ptr<QueueType> bufferQueue;
  size_t bufferSize;
};

BufferPool::BufferPool(const std::shared_ptr<IBufferFactory>& bufferFactory)
    : mImpl(make_shared<BufferPool::Impl>(bufferFactory)) {}

Buffer BufferPool::get(size_t bufferSize) {
  if (bufferSize == 0) {
    return Buffer();
  }

  if (bufferSize > mImpl->bufferSize) {
    mImpl->bufferQueue.reset();
    mImpl->bufferQueue = make_shared<Impl::QueueType>();
    mImpl->bufferSize = bufferSize;
  }

  Buffer sourceBuffer;
  if (!mImpl->bufferQueue->try_dequeue(sourceBuffer)) {
    sourceBuffer = mImpl->bufferFactory->Create(mImpl->bufferSize);
  }

  if (bufferSize > sourceBuffer.length) {
    throw runtime_error(
        "Buffer Pool internal error. Recycled buffer is too small.");
  }

  Buffer poolBuffer;
  const weak_ptr<Impl::QueueType> weakBufferQueue = mImpl->bufferQueue;
  poolBuffer.length = bufferSize;
  poolBuffer.data = shared_ptr<uint8_t[]>(
      sourceBuffer.data.get(),
      [weakBufferQueue, origBuffer {sourceBuffer}](uint8_t* data) {
        const auto bufferQueue = weakBufferQueue.lock();

        if (bufferQueue == nullptr) {
          return;
        }

        bufferQueue->enqueue(origBuffer);
      });

  return poolBuffer;
}

}  // namespace maplang
