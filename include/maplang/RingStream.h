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
#ifndef MAPLANG_RINGSTREAM_H_
#define MAPLANG_RINGSTREAM_H_

#include "maplang/IBufferFactory.h"

namespace maplang {

class RingStream final {
 public:
  explicit RingStream(
      const std::shared_ptr<const IBufferFactory>& bufferFactory,
      std::optional<size_t> initialSize = {});

  RingStream(RingStream&&) = default;

  size_t Read(void* buffer, size_t bufferSize);
  void Write(const void* buffer, size_t bufferSize);
  void Clear();
  size_t GetAvailableByteCount() const;

  RingStream& operator=(RingStream&&) = default;

 private:
  static const size_t kDefaultInitialBufferSize;

  const std::shared_ptr<const IBufferFactory> mBufferFactory;

  Buffer mBuffer;
  size_t mOffset;
  size_t mLength;

 private:
  RingStream(const RingStream&);
  RingStream& operator=(const RingStream&);

  void ResizeBuffer(size_t minimumBufferSize);
};

}  // namespace maplang

#include "maplang/RingStream-inl.h"

#endif  // MAPLANG_RINGSTREAM_H_
