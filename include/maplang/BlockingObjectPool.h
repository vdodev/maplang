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

#ifndef MAPLANG_BLOCKINGOBJECTPOOL_H_
#define MAPLANG_BLOCKINGOBJECTPOOL_H_

#include <functional>
#include <memory>

#include "maplang/blockingconcurrentqueue.h"

namespace maplang {

template <class T>
class BlockingObjectPool final {
 public:
  using Allocator = std::function<std::shared_ptr<T>()>;

 public:
  explicit BlockingObjectPool(size_t objectsInPool);

  void setAllocator(Allocator&& allocator);
  void setAllocator(const Allocator& allocator);

  std::shared_ptr<T> get();

 private:
  using QueueType = moodycamel::BlockingConcurrentQueue<std::shared_ptr<T>>;

 private:
  const size_t maxAllocatedObjects_;

  Allocator allocator_;
  std::shared_ptr<QueueType> objectQueue_;
  size_t totalAllocatedObjects_;
};

template <class T>
BlockingObjectPool<T>::BlockingObjectPool(size_t objectsInPool)
    : maxAllocatedObjects_(objectsInPool), totalAllocatedObjects_(0) {}

template <class T>
void BlockingObjectPool<T>::setAllocator(Allocator&& allocator) {
  objectQueue_.reset();
  objectQueue_ = std::make_shared<QueueType>();
  totalAllocatedObjects_ = 0;
  allocator_ = allocator;
}

template <class T>
void BlockingObjectPool<T>::setAllocator(const Allocator& allocator) {
  objectQueue_.reset();
  objectQueue_ = std::make_shared<QueueType>();
  totalAllocatedObjects_ = 0;
  allocator_ = allocator;
}

template <class T>
std::shared_ptr<T> BlockingObjectPool<T>::get() {
  std::shared_ptr<T> sourceObject;
  if (totalAllocatedObjects_ < maxAllocatedObjects_) {
    if (allocator_ == nullptr) {
      throw std::runtime_error(
          "BlockingObjectPool Allocator was not set before requesting frames.");
    }

    sourceObject = allocator_();
    totalAllocatedObjects_++;
  } else {
    objectQueue_->wait_dequeue(sourceObject);
  }

  const std::weak_ptr<QueueType> weakObjectQueue = objectQueue_;
  std::shared_ptr<T> poolObject(
      sourceObject.get(),
      [weakObjectQueue, origObject {sourceObject}](T* data) {
        const auto objectQueue = weakObjectQueue.lock();

        if (objectQueue == nullptr) {
          return;
        }

        objectQueue->enqueue(origObject);
      });

  return poolObject;
}

}  // namespace maplang

#endif  // MAPLANG_BLOCKINGOBJECTPOOL_H_
