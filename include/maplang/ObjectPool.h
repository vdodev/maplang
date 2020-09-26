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

#ifndef __MAPLANG_OBJECTPOOL_H__
#define __MAPLANG_OBJECTPOOL_H__

#include "maplang/Buffer.h"
#include <stack>

namespace maplang {

template<class T>
class ObjectPool final {
 public:
  ObjectPool(std::function<T* ()>&& objectFactory, std::function<void (T* obj)>&& disposer)
    : mFactory(move(objectFactory)), mDisposer(move(disposer)) {}

  ~ObjectPool() {
    printf("~ObjectPool()\n");

    while (!mObjects.empty()) {
      mDisposer(mObjects.top());
      mObjects.pop();
    }
  }

  T* get() {
    if (mObjects.empty()) {
      return mFactory();
    } else {
      T* top = mObjects.top();
      mObjects.pop();

      return top;
    }
  }

  void returnToPool(T* item) {
    mObjects.push(item);
  }

 private:
  const std::function<T* ()> mFactory;
  const std::function<void (T* obj)> mDisposer;

  std::stack<T*> mObjects;
};

}  // namespace maplang

#endif // __MAPLANG_OBJECTPOOL_H__
