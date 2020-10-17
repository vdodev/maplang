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

#ifndef SR_TRANSCODE__FRAMEPOOL_H_
#define SR_TRANSCODE__FRAMEPOOL_H_

#include <functional>
#include <memory>

namespace maplang {

class BufferPool final {
 public:
  using Allocator = std::function<uint8_t* (size_t length)>;
  using Deallocator = std::function<void (uint8_t* buffer)>;

  BufferPool();

  void setAllocator(Allocator&& allocator, Deallocator&& deallocator);
  void setAllocator(const Allocator& allocator, const Deallocator& deallocator);

  std::shared_ptr<uint8_t> get(size_t minimumSize);

 private:
  struct Impl;
  const std::shared_ptr<Impl> mImpl;
};

}  // namespace maplang

#endif  // SR_TRANSCODE__FRAMEPOOL_H_
