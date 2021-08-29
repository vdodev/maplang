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
#ifndef MAPLANG__RINGSTREAM_INL_H_
#define MAPLANG__RINGSTREAM_INL_H_

#include <cstring>

namespace maplang {

const size_t RingStream::kDefaultInitialBufferSize = 1024;

RingStream::RingStream(
    const std::shared_ptr<const IBufferFactory>& bufferFactory,
    std::optional<size_t> initialSize)
    : mBuffer(
        bufferFactory->Create(initialSize.value_or(kDefaultInitialBufferSize))),
      mOffset(0), mLength(0) {}

size_t RingStream::Read(void* buffer, size_t bufferSize) {
  uint8_t* __restrict destination = reinterpret_cast<uint8_t*>(buffer);
  uint8_t* __restrict source = mBuffer.data.get() + mOffset;

  const size_t byteCountToRead = std::min(mLength, bufferSize);
  size_t remainingByteCountToRead = byteCountToRead;

  while (remainingByteCountToRead > 0) {
    size_t byteCountToEndOfBuffer = mBuffer.length - mOffset;

    const size_t byteCountToCopy =
        std::min(byteCountToEndOfBuffer, remainingByteCountToRead);

    memcpy(destination, source, byteCountToCopy);

    destination += byteCountToCopy;
    mOffset = (mOffset + byteCountToCopy) % mBuffer.length;
    mLength -= byteCountToCopy;

    source = mBuffer.data.get() + mOffset;
  }

  return byteCountToRead;
}

void RingStream::Write(const void* buffer, size_t bufferSize) {
  const size_t minBufferSize = mLength + bufferSize;

  if (minBufferSize < mBuffer.length) {
    ResizeBuffer(minBufferSize);
  }

  const uint8_t* __restrict source = reinterpret_cast<const uint8_t*>(buffer);
  uint8_t* __restrict destination = mBuffer.data.get();

  size_t remainingByteCountToWrite = bufferSize;

  while (remainingByteCountToWrite > 0) {
    size_t byteCountToEndOfBuffer = mBuffer.length - mOffset;
    size_t copyByteCount =
        std::min(byteCountToEndOfBuffer, remainingByteCountToWrite);

    memcpy(destination, source, copyByteCount);

    remainingByteCountToWrite -= copyByteCount;
    mLength += copyByteCount;
    destination = mBuffer.data.get() + (mOffset + mLength) % mBuffer.length;
    source += copyByteCount;
  }
}

void RingStream::Clear() {
  mOffset = 0;
  mLength = 0;
}

size_t RingStream::GetAvailableByteCount() const { return mLength; }

void RingStream::ResizeBuffer(size_t minimumBufferSize) {
  size_t newBufferSize = mBuffer.length;

  do {
    newBufferSize *= 2;
  } while (newBufferSize < minimumBufferSize);

  Buffer newBuffer = mBufferFactory->Create(newBufferSize);

  const bool bufferWraps = mOffset + mLength > mBuffer.length;

  if (bufferWraps) {
    uint8_t* __restrict originalTail = mBuffer.data.get() + mOffset;

    const size_t tailLength = mBuffer.length - mOffset;
    const size_t newOffset = newBufferSize - tailLength;
    uint8_t* __restrict newTail = mBuffer.data.get() + newOffset;

    memcpy(newTail, originalTail, tailLength);

    mOffset = newOffset;
  }

  std::swap(newBuffer, mBuffer);
}

}  // namespace maplang

#endif  // MAPLANG__RINGSTREAM_INL_H_