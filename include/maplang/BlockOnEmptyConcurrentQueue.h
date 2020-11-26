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

#ifndef MAPLANG__BLOCKONEMPTYCONCURRENTQUEUE_H_
#define MAPLANG__BLOCKONEMPTYCONCURRENTQUEUE_H_

#include <condition_variable>
#include <mutex>

#include "concurrentqueue.h"

namespace maplang {

template <class T>
class BlockOnEmptyConcurrentQueue final {
 public:
  BlockOnEmptyConcurrentQueue();
  ~BlockOnEmptyConcurrentQueue();

  void push(const T& t);
  void push(T&& t);

  /// Returns false if the queue was destroyed before an item was received.
  bool pop(T& t);

 private:
  struct Sync {
    std::mutex lock;
    std::condition_variable itemAddedCv;
  };

  std::shared_ptr<moodycamel::ConcurrentQueue<T>> mQueue;
  std::shared_ptr<Sync> mSync;
};

template <class T>
BlockOnEmptyConcurrentQueue<T>::BlockOnEmptyConcurrentQueue()
    : mQueue(std::make_shared<moodycamel::ConcurrentQueue<T>>()),
      mSync(std::make_shared<Sync>()) {}

template <class T>
BlockOnEmptyConcurrentQueue<T>::~BlockOnEmptyConcurrentQueue() {
  mQueue.reset();
  mSync->itemAddedCv.notify_all();
}

template <class T>
void BlockOnEmptyConcurrentQueue<T>::push(const T& t) {
  mQueue->enqueue(t);
  mSync->itemAddedCv.notify_one();
}

template <class T>
void BlockOnEmptyConcurrentQueue<T>::push(T&& t) {
  mQueue->enqueue(move(t));
  mSync->itemAddedCv.notify_one();
}

template <class T>
bool BlockOnEmptyConcurrentQueue<T>::pop(T& item) {
  if (mQueue->try_dequeue(item)) {
    return true;
  }

  std::unique_lock<std::mutex> ul(mSync->lock);
  std::shared_ptr<Sync> sync = mSync;
  std::weak_ptr<moodycamel::ConcurrentQueue<T>> weakQueue = mQueue;

  bool dequeuedItem = false;
  mSync->itemAddedCv.wait(ul, [weakQueue, sync, &dequeuedItem, &item]() {
    const auto strongQueue = weakQueue.lock();
    if (strongQueue == nullptr) {
      dequeuedItem = false;

      const bool unblock = true;
      return unblock;
    }

    dequeuedItem = strongQueue->try_dequeue(item);

    const bool unblock = dequeuedItem;
    return unblock;
  });

  return dequeuedItem;
}

}  // namespace maplang

#endif  // MAPLANG__BLOCKONEMPTYCONCURRENTQUEUE_H_
