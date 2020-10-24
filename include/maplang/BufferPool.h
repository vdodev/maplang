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

#ifndef MAPLANG__BUFFERPOOL_H_
#define MAPLANG__BUFFERPOOL_H_

#include <functional>
#include <memory>

namespace maplang {

class BufferPool final {
 public:
  using Allocator = std::function<std::shared_ptr<uint8_t> (size_t length)>;

  BufferPool();

  void setAllocator(Allocator&& allocator);
  void setAllocator(const Allocator& allocator);

  std::shared_ptr<uint8_t> get(size_t minimumSize);

 private:
  struct Impl;
  const std::shared_ptr<Impl> mImpl;
};

}  // namespace maplang

#endif  // MAPLANG__BUFFERPOOL_H_
