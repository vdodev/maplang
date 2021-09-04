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

#include "maplang/BlockingBufferPool.h"

#include <stack>

#include "maplang/IBufferFactory.h"
#include "maplang/blockingconcurrentqueue.h"

using namespace std;

namespace maplang {

struct BlockingBufferPool::Impl final {
  using QueueType = moodycamel::BlockingConcurrentQueue<Buffer>;

  Impl(
      const shared_ptr<const IBufferFactory>& _bufferFactory,
      size_t maxAllocatedBuffers)
      : maxAllocatedBuffers(maxAllocatedBuffers), bufferSize(0),
        bufferFactory(_bufferFactory), totalAllocatedBuffers(0) {}

  const size_t maxAllocatedBuffers;

  const shared_ptr<const IBufferFactory> bufferFactory;
  shared_ptr<QueueType> bufferQueue;
  size_t bufferSize;
  size_t totalAllocatedBuffers;
};

BlockingBufferPool::BlockingBufferPool(
    const shared_ptr<const IBufferFactory>& bufferFactory,
    size_t maxAllocatedBuffers)
    : mImpl(make_shared<BlockingBufferPool::Impl>(
        bufferFactory,
        maxAllocatedBuffers)) {}

Buffer BlockingBufferPool::get(size_t bufferSize) {
  if (bufferSize == 0) {
    return Buffer();
  }

  if (bufferSize > mImpl->bufferSize) {
    mImpl->bufferQueue.reset();
    mImpl->bufferQueue = make_shared<Impl::QueueType>();
    mImpl->bufferSize = bufferSize;
    mImpl->totalAllocatedBuffers = 0;
  }

  Buffer sourceBuffer;
  if (mImpl->totalAllocatedBuffers < mImpl->maxAllocatedBuffers) {
    sourceBuffer = mImpl->bufferFactory->Create(mImpl->bufferSize);
    mImpl->totalAllocatedBuffers++;
  } else {
    mImpl->bufferQueue->wait_dequeue(sourceBuffer);
  }

  if (bufferSize > sourceBuffer.length) {
    throw runtime_error(
        "Buffer Pool internal error. Recycled buffer is too small.");
  }

  Buffer poolBuffer;
  const weak_ptr<Impl::QueueType> weakBufferQueue = mImpl->bufferQueue;
  poolBuffer.length = bufferSize;
  poolBuffer.data = shared_ptr<uint8_t>(
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
