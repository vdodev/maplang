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

#include "maplang/MemoryStream.h"

#include <cstring>
#include <stdexcept>

using namespace std;

namespace maplang {

const size_t MemoryStream::kNotFound = SIZE_MAX;

void MemoryStream::append(const Buffer& buffer) {
  if (buffer.length == 0) {
    return;
  }

  if (buffer.data == nullptr) {
    throw runtime_error("Data length is > 0, but buffer is NULL.");
  }

  mBuffers.push_back(buffer);
  mSize += buffer.length;
}

void MemoryStream::clear() {
  mSize = 0;
  mBuffers.clear();
}

std::string MemoryStream::asString() const {
  std::ostringstream stream;
  visitBuffers(0, [&stream](size_t bufferIndex, Buffer&& buffer) {
    stream.write(reinterpret_cast<const char*>(buffer.data.get()), buffer.length);
    return true;
  });

  return stream.str();
}

uint8_t MemoryStream::byteAt(size_t index) const {
  size_t bufferIndex = 0;
  size_t bufferOffset = 0;

  findIndex(index, &bufferIndex, &bufferOffset);
  return mBuffers[bufferIndex].data.get()[bufferOffset];
}

size_t MemoryStream::firstIndexOf(uint8_t findThis, size_t startOffset,
                                  size_t endOffset) const {
  if (endOffset > mSize) {
    endOffset = mSize;
  }

  if (endOffset <= startOffset) {
    return kNotFound;
  }

  size_t foundAtIndex = kNotFound;
  size_t lengthOfPreviousBuffers = 0;
  visitBuffers(startOffset, endOffset,
               [findThis, &foundAtIndex, &lengthOfPreviousBuffers](
                   size_t bufferIndex, Buffer&& buffer) {
                 const void* foundAt =
                     memchr(buffer.data.get(), findThis, buffer.length);

                 if (foundAt == nullptr) {
                   lengthOfPreviousBuffers += buffer.length;
                   return true;  // keep iterating through buffers.
                 }

                 const size_t offsetInCurrentBuffer =
                     reinterpret_cast<intptr_t>(foundAt) -
                     reinterpret_cast<intptr_t>(buffer.data.get());
                 foundAtIndex = lengthOfPreviousBuffers + offsetInCurrentBuffer;
                 return false;  // Stop iterating through buffers
               });

  if (foundAtIndex == kNotFound) {
    return kNotFound;
  }

  return startOffset + foundAtIndex;
}

size_t MemoryStream::lastIndexOfAnyInSet(const void* findAnyOfTheseBytes,
                                         size_t findAnyOfTheseBytesLength,
                                         size_t startOffset,
                                         size_t endOffset) const {
  if (endOffset > mSize) {
    endOffset = mSize;
  }

  if (startOffset >= endOffset) {
    return kNotFound;
  }

  for (size_t i = endOffset - 1;; i--) {
    if (memchr(findAnyOfTheseBytes, byteAt(i), findAnyOfTheseBytesLength) !=
        nullptr) {
      return i;
    }

    if (i == startOffset) {
      return kNotFound;
    }
  }
}

size_t MemoryStream::lastIndexNotOfAnyInSet(const void* findAnyOfTheseBytes,
                                            size_t findAnyOfTheseBytesLength,
                                            size_t startOffset,
                                            size_t endOffset) const {
  if (endOffset > mSize) {
    endOffset = mSize;
  }

  if (startOffset >= endOffset) {
    return kNotFound;
  }

  for (size_t i = endOffset - 1;; i--) {
    if (memchr(findAnyOfTheseBytes, byteAt(i), findAnyOfTheseBytesLength) ==
        nullptr) {
      return i;
    }

    if (i == startOffset) {
      return kNotFound;
    }
  }
}

size_t MemoryStream::firstIndexOfAnyInSet(const void* findAnyOfTheseBytes,
                                          size_t findAnyOfTheseBytesLength,
                                          size_t startOffset,
                                          size_t endOffset) const {
  if (endOffset > mSize) {
    endOffset = mSize;
  }

  if (startOffset >= endOffset) {
    return kNotFound;
  }

  for (size_t i = startOffset; i < endOffset; i++) {
    if (memchr(findAnyOfTheseBytes, byteAt(i), findAnyOfTheseBytesLength) !=
        nullptr) {
      return i;
    }
  }

  return kNotFound;
}

size_t MemoryStream::firstIndexNotOfAnyInSet(const void* findAnyOfTheseBytes,
                                             size_t findAnyOfTheseBytesLength,
                                             size_t startOffset,
                                             size_t endOffset) const {
  if (endOffset > mSize) {
    endOffset = mSize;
  }

  if (startOffset >= endOffset) {
    return kNotFound;
  }

  for (size_t i = startOffset; i < endOffset; i++) {
    if (memchr(findAnyOfTheseBytes, byteAt(i), findAnyOfTheseBytesLength) ==
        nullptr) {
      return i;
    }
  }

  return kNotFound;
}

size_t MemoryStream::firstIndexOf(const void* findThis, size_t findThisLength,
                                  size_t startOffset, size_t endOffset) const {
  if (endOffset > mSize) {
    endOffset = mSize;
  }

  if (findThisLength == 0) {
    return 0;
  }

  if (startOffset >= endOffset) {
    return kNotFound;
  }

  const uint8_t* const findThisU8 = static_cast<const uint8_t*>(findThis);
  const uint8_t findThisFirstByte = findThisU8[0];

  while (true) {
    size_t candidateOffset =
        firstIndexOf(findThisFirstByte, startOffset, endOffset);
    if (candidateOffset == kNotFound) {
      return kNotFound;
    } else if (equals(findThisU8 + 1, findThisLength - 1,
                      candidateOffset + 1)) {
      return candidateOffset;
    } else {
      startOffset++;
    }
  }
}

bool MemoryStream::equals(const Buffer& data, size_t streamOffset) const {
  return equals(data.data.get(), data.length, streamOffset);
}

bool MemoryStream::equalsString(const string& str, size_t streamOffset) const {
  return equals(str.c_str(), str.length(), streamOffset);
}

bool MemoryStream::equals(const void* data, size_t compareLength,
                          size_t streamOffset) const {
  if (streamOffset + compareLength > mSize) {
    return false;
  }

  const uint8_t* dataU8 = static_cast<const uint8_t*>(data);

  size_t compareIndex;
  for (compareIndex = 0; compareIndex < compareLength; compareIndex++) {
    if (dataU8[compareIndex] != byteAt(streamOffset + compareIndex)) {
      return false;
    }
  }

  return compareIndex == compareLength;
}

void MemoryStream::split(char separator, const OnFragment& onFragment,
                         size_t maxTokens) const {
  split(&separator, 1, onFragment, maxTokens);
}

void MemoryStream::split(uint8_t separator, const OnFragment& onFragment,
                         size_t maxTokens) const {
  split(&separator, 1, onFragment, maxTokens);
}

void MemoryStream::split(const void* separator, size_t separatorLength,
                         const OnFragment& onFragment, size_t maxTokens) const {
  size_t startOffset = 0;
  for (size_t fragmentIndex = 0;; fragmentIndex++) {
    size_t endOffset;
    if (fragmentIndex == maxTokens - 1) {
      endOffset = mSize;
    } else {
      endOffset = firstIndexOf(separator, separatorLength, startOffset,
                               mSize - startOffset);
      if (endOffset == kNotFound) {
        endOffset = mSize;
      }
    }

    onFragment(fragmentIndex, subStream(startOffset, endOffset));

    startOffset = endOffset + separatorLength;

    if (endOffset == mSize) {
      break;
    }
  }
}

vector<string> MemoryStream::splitIntoStrings(const void* separator, size_t separatorLength, size_t maxTokens) const {
  vector<string> tokens;

  split(
      separator,
      separatorLength,
      [&tokens](size_t fragmentIndex, MemoryStream&& stream) {
    tokens.push_back(stream.asString());
    return true;
  }, maxTokens);

  return tokens;
}

vector<string> MemoryStream::splitIntoStrings(char separator, size_t maxTokens) const {
  return splitIntoStrings(&separator, 1, maxTokens);
}

vector<MemoryStream> MemoryStream::splitIntoMemoryStreams(
    const void* separator, size_t separatorLength, size_t maxTokens) const {
  vector<MemoryStream> tokens;

  split(
      separator,
      separatorLength,
      [&tokens](size_t fragmentIndex, MemoryStream&& stream) {
        tokens.push_back(move(stream));
        return true;
      }, maxTokens);

  return tokens;
}

vector<MemoryStream> MemoryStream::splitIntoMemoryStreams(char separator, size_t maxTokens) const {
  return splitIntoMemoryStreams(&separator, 1, maxTokens);
}

vector<MemoryStream> MemoryStream::splitIntoMemoryStreams(uint8_t separator, size_t maxTokens) const {
  return splitIntoMemoryStreams(&separator, 1, maxTokens);
}

MemoryStream MemoryStream::subStream(size_t startOffset,
                                     size_t endOffset) const {
  MemoryStream stream;

  visitBuffers(startOffset, endOffset,
               [&stream](size_t bufferIndex, Buffer&& buffer) {
                 stream.append(buffer);
                 return true;
               });

  return stream;
}

void MemoryStream::visitBuffers(const OnBuffer& onBuffer) const {
  for (size_t i = 0; i < mBuffers.size(); i++) {
    Buffer buffer = mBuffers[i];
    if (!onBuffer(i, move(buffer))) {
      break;
    }
  }
}

void MemoryStream::visitBuffers(size_t startOffset,
                                const OnBuffer& onBuffer) const {
  visitBuffers(startOffset, mSize, onBuffer);
}

void MemoryStream::visitBuffers(size_t startOffset, size_t endOffset,
                                const OnBuffer& onBuffer) const {
  if (endOffset > mSize) {
    endOffset = mSize;
  }

  if (startOffset >= endOffset) {
    return;
  }

  size_t firstBufferIndex;
  size_t firstBufferStartOffset;
  findIndex(startOffset, &firstBufferIndex, &firstBufferStartOffset);

  size_t lastBufferIndex;
  size_t lastBufferEndOffset;
  findIndex(endOffset - 1, &lastBufferIndex, &lastBufferEndOffset);
  lastBufferEndOffset++;

  for (size_t bufferIndex = firstBufferIndex; bufferIndex <= lastBufferIndex;
       bufferIndex++) {
    const Buffer& buffer = mBuffers[bufferIndex];

    size_t bufferStartOffset = 0;
    size_t bufferEndOffset = buffer.length;

    if (bufferIndex == firstBufferIndex) {
      bufferStartOffset = firstBufferStartOffset;
    }

    if (bufferIndex == lastBufferIndex) {
      bufferEndOffset = lastBufferEndOffset;
    }

    Buffer sendBuffer;
    sendBuffer.data =
        shared_ptr<uint8_t>(buffer.data, buffer.data.get() + bufferStartOffset);
    sendBuffer.length = bufferEndOffset - bufferStartOffset;

    if (!onBuffer(bufferIndex, move(sendBuffer))) {
      break;
    }
  }
}

MemoryStream MemoryStream::trim() const {
  static constexpr char whitespace[] = " \r\n\t";
  static constexpr size_t NUM_WHITE_SPACE_CHARS =
      sizeof(whitespace) - 1;  // -1 for null-terminator

  const size_t firstNonWhitespaceIndex =
      firstIndexNotOfAnyInSet(whitespace, NUM_WHITE_SPACE_CHARS);
  const size_t lastNonWhitespaceIndex =
      lastIndexNotOfAnyInSet(whitespace, NUM_WHITE_SPACE_CHARS);

  if (firstNonWhitespaceIndex == kNotFound) {
    return MemoryStream();
  }

  return subStream(firstNonWhitespaceIndex, lastNonWhitespaceIndex + 1);
}

void MemoryStream::findIndex(size_t byteIndex, size_t* bufferIndex,
                             size_t* offsetInBuffer) const {
  size_t lengthOfPreviousBuffers = 0;

  const size_t bufferCount = mBuffers.size();
  for (size_t i = 0; i < bufferCount; i++) {
    const auto& buffer = mBuffers[i];

    if (byteIndex >= lengthOfPreviousBuffers &&
        byteIndex < lengthOfPreviousBuffers + buffer.length) {
      *bufferIndex = i;
      *offsetInBuffer = byteIndex - lengthOfPreviousBuffers;
      return;
    }

    lengthOfPreviousBuffers += buffer.length;
  }

  throw runtime_error("Index is out of bounds");
}

size_t MemoryStream::size() const { return mSize; }

size_t MemoryStream::read(size_t streamOffset, size_t numberOfBytesToRead,
                          Buffer* buffer) const {
  return read(streamOffset, numberOfBytesToRead, buffer->data.get(),
              buffer->length);
}

size_t MemoryStream::read(size_t streamOffset, size_t numberOfBytesToRead,
                          void* copyToBuffer, size_t bufferLength) const {
  size_t readByteCount = 0;
  uint8_t* copyToBufferU8 = reinterpret_cast<uint8_t*>(copyToBuffer);

  visitBuffers(streamOffset, streamOffset + numberOfBytesToRead,
               [&readByteCount, copyToBufferU8](size_t index, Buffer&& buffer) {
                 memcpy(copyToBufferU8 + readByteCount, buffer.data.get(),
                        buffer.length);
                 readByteCount += buffer.length;
                 return true;
               });

  return readByteCount;
}

}  // namespace maplang
