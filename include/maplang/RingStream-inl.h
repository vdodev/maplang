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
#ifndef MAPLANG_RINGSTREAM_INL_H_
#define MAPLANG_RINGSTREAM_INL_H_

#include <cstring>

namespace maplang {

const size_t RingStream::kDefaultInitialBufferSize = 1024;

RingStream::RingStream(
    const std::shared_ptr<const IBufferFactory>& bufferFactory,
    std::optional<size_t> initialSize)
    : mBufferFactory(bufferFactory),
      mBuffer(bufferFactory->Create(
          initialSize.value_or(kDefaultInitialBufferSize))),
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
    remainingByteCountToRead -= byteCountToCopy;

    source = mBuffer.data.get() + mOffset;
  }

  return byteCountToRead;
}

size_t RingStream::Skip(size_t skipByteCount) {
  if (skipByteCount > mLength) {
    const size_t skippedByteCount = mLength;

    mOffset = 0;
    mLength = 0;

    return skippedByteCount;
  } else {
    mOffset = (mOffset + skipByteCount) % mBuffer.length;

    return skipByteCount;
  }
}

void RingStream::Write(const void* buffer, size_t bufferSize) {
  const size_t minBufferSize = mLength + bufferSize;

  if (minBufferSize < mBuffer.length) {
    ResizeBuffer(minBufferSize);
  }

  const uint8_t* __restrict source = reinterpret_cast<const uint8_t*>(buffer);
  size_t remainingByteCountToWrite = bufferSize;

  while (remainingByteCountToWrite > 0) {
    size_t offsetToUnusedBuffer = (mOffset + mLength) % mBuffer.length;
    size_t byteCountToEndOfBuffer = mBuffer.length - offsetToUnusedBuffer;
    size_t copyByteCount =
        std::min(byteCountToEndOfBuffer, remainingByteCountToWrite);

    uint8_t* __restrict destination = mBuffer.data.get() + offsetToUnusedBuffer;
    memcpy(destination, source, copyByteCount);

    remainingByteCountToWrite -= copyByteCount;
    mLength += copyByteCount;
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
  } else {
    memcpy(newBuffer.data.get(), mBuffer.data.get() + mOffset, mLength);
    std::swap(newBuffer, mBuffer);
    mOffset = 0;
  }

  std::swap(newBuffer, mBuffer);
}

}  // namespace maplang

#endif  // MAPLANG_RINGSTREAM_INL_H_
