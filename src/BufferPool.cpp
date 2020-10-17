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


struct AllocDealloc final {
  BufferPool::Allocator allocator;
  BufferPool::Deallocator deallocator;
};

class QueuedBuffer final {
 public:
  uint8_t* buffer;

 public:
  QueuedBuffer() : buffer(nullptr), stolen(false) {}

  QueuedBuffer(uint8_t* buffer, const shared_ptr<AllocDealloc>& allocDealloc) : buffer(buffer), allocDealloc(allocDealloc), stolen(false) {}
  QueuedBuffer(QueuedBuffer&& b) {
    allocDealloc = move(b.allocDealloc);
    stolen = b.stolen;
    buffer = b.buffer;

    b.steal();
  }

  QueuedBuffer(const QueuedBuffer& b) = delete;

  ~QueuedBuffer() {
    clear();
  }

  void steal() {
    stolen = true;
  }

  QueuedBuffer& operator=(const QueuedBuffer& b) = delete;

  QueuedBuffer& operator=(QueuedBuffer&& b) {
    clear();

    allocDealloc = move(b.allocDealloc);
    stolen = b.stolen;
    buffer = b.buffer;

    b.steal();

    return *this;
  }

  void clear() {
    if (!stolen && buffer != nullptr && allocDealloc->deallocator != nullptr) {
      allocDealloc->deallocator(buffer);
    }
  }

 private:
  shared_ptr<AllocDealloc> allocDealloc;
  bool stolen;
};

struct BufferPool::Impl final {
  using QueueType = moodycamel::ConcurrentQueue<QueuedBuffer>;

  Impl() : bufferSize(0), allocDealloc(make_shared<AllocDealloc>()) {}

  shared_ptr<AllocDealloc> allocDealloc;
  shared_ptr<QueueType> bufferQueue;
  size_t bufferSize;
};

BufferPool::BufferPool() : mImpl(make_shared<BufferPool::Impl>()) {}

void BufferPool::setAllocator(Allocator&& allocator, Deallocator&& deallocator) {
  mImpl->bufferQueue.reset();
  mImpl->bufferSize = 0;

  shared_ptr<AllocDealloc> allocDealloc = make_shared<AllocDealloc>();
  allocDealloc->allocator = move(allocator);
  allocDealloc->deallocator = move(deallocator);

  mImpl->allocDealloc = allocDealloc;
}

void BufferPool::setAllocator(const Allocator& allocator, const Deallocator& deallocator) {
  mImpl->bufferQueue.reset();
  mImpl->bufferSize = 0;

  shared_ptr<AllocDealloc> allocDealloc = make_shared<AllocDealloc>();
  allocDealloc->allocator = allocator;
  allocDealloc->deallocator = deallocator;

  mImpl->allocDealloc = allocDealloc;
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

  QueuedBuffer bufferFromQueue;
  uint8_t* rawBuffer = nullptr;

  if (mImpl->bufferQueue->try_dequeue(bufferFromQueue)) {
    rawBuffer = bufferFromQueue.buffer;
    bufferFromQueue.steal();
  } else {
    if (mImpl->allocDealloc->allocator == nullptr) {
      throw runtime_error("BufferPool Allocator was not set before requesting frames.");
    }

    rawBuffer = mImpl->allocDealloc->allocator(bufferSize);
    if (rawBuffer == nullptr) {
      throw bad_alloc();
    }
  }

  const weak_ptr<Impl::QueueType> weakBufferQueue = mImpl->bufferQueue;
  const shared_ptr<AllocDealloc> allocDealloc = mImpl->allocDealloc;

  return shared_ptr<uint8_t>(rawBuffer, [weakBufferQueue, allocDealloc](uint8_t* rawBuffer) {
    const auto bufferQueue = weakBufferQueue.lock();

    if (bufferQueue == nullptr) {
      if (allocDealloc->deallocator != nullptr) {
        allocDealloc->deallocator(rawBuffer);
      }

      return;
    }

    bufferQueue->enqueue(QueuedBuffer(rawBuffer, allocDealloc));
  });
}

}  // namespace maplang
