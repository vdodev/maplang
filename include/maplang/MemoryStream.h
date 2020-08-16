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

#ifndef __MAPLANG_MEMORYSTREAM_H__
#define __MAPLANG_MEMORYSTREAM_H__

#include <climits>
#include <ostream>
#include <vector>
#include <functional>
#include <istream>
#include <sstream>

#include "maplang/Buffer.h"

namespace maplang {

class MemoryStream final {
 public:
  static const size_t kNotFound;

  void append(const Buffer& buffer);

  size_t read(size_t streamOffset, size_t numberOfBytesToRead, void* buffer,
              size_t bufferLength) const;
  size_t read(size_t streamOffset, size_t numberOfBytesToRead,
              Buffer* buffer) const;

  uint8_t byteAt(size_t index) const;

  size_t firstIndexOf(uint8_t value, size_t startOffset = 0,
                      size_t endOffset = SIZE_MAX) const;
  size_t firstIndexOf(const void* findThis, size_t findThisLength,
                      size_t startOffset = 0,
                      size_t endOffset = SIZE_MAX) const;

  size_t firstIndexOfAnyInSet(const void* findAnyOfTheseBytes,
                              size_t findAnyOfTheseBytesLength,
                              size_t startOffset = 0,
                              size_t endOffset = SIZE_MAX) const;
  size_t lastIndexOfAnyInSet(const void* findAnyOfTheseBytes,
                             size_t findAnyOfTheseBytesLength,
                             size_t startOffset = 0,
                             size_t endOffset = SIZE_MAX) const;

  size_t firstIndexNotOfAnyInSet(const void* findAnyOfTheseBytes,
                                 size_t findAnyOfTheseBytesLength,
                                 size_t startOffset = 0,
                                 size_t endOffset = SIZE_MAX) const;
  size_t lastIndexNotOfAnyInSet(const void* findAnyOfTheseBytes,
                                size_t findAnyOfTheseBytesLength,
                                size_t startOffset = 0,
                                size_t endOffset = SIZE_MAX) const;

  MemoryStream subStream(size_t startOffset,
                         size_t endOffset = SIZE_MAX) const;

  using OnBuffer = std::function<bool(size_t bufferIndex, Buffer&& buffer)>;
  void visitBuffers(const OnBuffer& onBuffer) const;
  void visitBuffers(size_t startOffset, const OnBuffer& onBuffer) const;
  void visitBuffers(size_t startOffset, size_t endOffset,
                    const OnBuffer& onBuffer) const;

  bool equals(const void* data, size_t compareLength,
              size_t streamOffset = 0) const;
  bool equals(const Buffer& buffer, size_t streamOffset = 0) const;
  bool equalsString(const std::string& str, size_t streamOffset = 0) const;

  using OnFragment =
      std::function<bool(size_t fragmentIndex, MemoryStream&& fragmentStream)>;
  void split(uint8_t separator, const OnFragment& onFragment,
             size_t maxTokens = SIZE_MAX) const;
  void split(char separator, const OnFragment& onFragment,
             size_t maxTokens = SIZE_MAX) const;
  void split(const void* separator, size_t separatorLength,
             const OnFragment& onFragment, size_t maxTokens = SIZE_MAX) const;

  std::vector<std::string> splitIntoStrings(const void* separator, size_t separatorLength, size_t maxTokens = SIZE_MAX) const;
  std::vector<std::string> splitIntoStrings(char separator, size_t maxTokens = SIZE_MAX) const;

  std::vector<MemoryStream> splitIntoMemoryStreams(const void* separator, size_t separatorLength, size_t maxTokens = SIZE_MAX) const;
  std::vector<MemoryStream> splitIntoMemoryStreams(char separator, size_t maxTokens = SIZE_MAX) const;
  std::vector<MemoryStream> splitIntoMemoryStreams(uint8_t separator, size_t maxTokens = SIZE_MAX) const;


  MemoryStream trim() const;

  size_t size() const;
  void clear();

  std::string asString() const;

 private:
  std::vector<Buffer> mBuffers;
  size_t mSize = 0;

  void findIndex(size_t byteIndex, size_t* bufferIndex,
                 size_t* offsetInBuffer) const;
};

class MemoryIStream final : public std::basic_istream<uint8_t> {
 public:
  MemoryIStream(const maplang::MemoryStream& stream);

};

}  // namespace maplang

inline std::ostream& operator<<(std::ostream& output,
                                const maplang::MemoryStream& memoryStream) {
  memoryStream.visitBuffers([&output](size_t index, maplang::Buffer&& buffer) {
    output.write(reinterpret_cast<const char*>(buffer.data.get()),
                 buffer.length);
    return true;
  });

  return output;
}

#endif  // __MAPLANG_MEMORYSTREAM_H__
