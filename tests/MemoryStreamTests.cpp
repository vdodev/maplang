/*
 *Copyright 2020 VDO Dev Inc <support@maplang.com>
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

#include "gtest/gtest.h"
#include "maplang/MemoryStream.h"

using namespace std;

namespace maplang {

TEST(WhenNoBuffersAreAdded, VisitBuffersDoesntReturnAnyBuffers) {
  MemoryStream stream;
  Buffer emptyBuffer;
  stream.append(emptyBuffer);

  ASSERT_EQ(0, stream.size());

  stream.visitBuffers([](size_t index, Buffer&& buffer) -> bool {
    ADD_FAILURE();
    return false;
  });
}

TEST(WhenABufferWithASingleByteIsAdded, TheStreamHasOneByte) {
  MemoryStream stream;
  Buffer buffer;
  buffer.data =
      shared_ptr<uint8_t>(new uint8_t[1], default_delete<uint8_t[]>());
  buffer.length = 1;
  stream.append(buffer);

  ASSERT_EQ(1, stream.size());

  stream.visitBuffers([](size_t index, Buffer&& buffer) -> bool {
    EXPECT_EQ(0, index);
    return true;
  });
}

TEST(WhenSeveralBuffersAreAdded, TheByteCountAndNumberOfBuffersIsCorrect) {
  MemoryStream stream;
  static constexpr size_t kBufferLength = 100;
  static constexpr size_t kBufferCount = 4;
  for (size_t i = 0; i < kBufferCount; i++) {
    Buffer buffer;
    buffer.data = shared_ptr<uint8_t>(
        new uint8_t[kBufferLength],
        default_delete<uint8_t[]>());
    buffer.length = kBufferLength;
    stream.append(buffer);
  }

  ASSERT_EQ(kBufferCount * kBufferLength, stream.size());

  size_t lastBufferIndex = SIZE_MAX;
  stream.visitBuffers(
      [&lastBufferIndex](size_t index, Buffer&& buffer) -> bool {
        lastBufferIndex = index;
        return true;
      });

  ASSERT_EQ(kBufferCount - 1, lastBufferIndex);
}

TEST(WhenSeveralBuffersAreAddedAndReadIntoARawBuffer, TheStreamDataIsCorrect) {
  MemoryStream stream;
  static constexpr size_t kBufferLength = 1;
  static constexpr size_t kBufferCount = 4;
  for (size_t i = 0; i < kBufferCount; i++) {
    Buffer buffer;
    buffer.data = shared_ptr<uint8_t>(
        new uint8_t[kBufferLength],
        default_delete<uint8_t[]>());
    buffer.data.get()[0] = i;
    buffer.length = kBufferLength;
    stream.append(buffer);
  }

  static constexpr size_t kStreamLength = kBufferLength * kBufferCount;
  ASSERT_EQ(kStreamLength, stream.size());

  uint8_t copiedFromStream[kStreamLength];
  size_t readBytes =
      stream.read(0, kStreamLength, copiedFromStream, kStreamLength);

  ASSERT_EQ(kStreamLength, readBytes);
  for (size_t i = 0; i < kStreamLength; i++) {
    ASSERT_EQ(i, copiedFromStream[i]);
  }
}

TEST(WhenSeveralBuffersAreConvertedToAStdString, TheStreamDataIsCorrect) {
  MemoryStream stream;
  stream.append(Buffer("a"));
  stream.append(Buffer("bc"));
  stream.append(Buffer("defg"));

  string streamString1 = stream.toString();
  ASSERT_STREQ("abcdefg", streamString1.c_str());

  string streamString2 = stream.toString(1);
  ASSERT_STREQ("bcdefg", streamString2.c_str());

  string streamString3 = stream.toString(1, 2);
  ASSERT_STREQ("b", streamString3.c_str());

  string streamString4 = stream.toString(1, 4);
  ASSERT_STREQ("bcd", streamString4.c_str());

  string streamString5 = stream.toString(2, 4);
  ASSERT_STREQ("cd", streamString5.c_str());

  string streamString6 = stream.toString(4);
  ASSERT_STREQ("efg", streamString6.c_str());
}

TEST(WhenSeveralBuffersAreAddedAndReadIntoABuffer, TheStreamDataIsCorrect) {
  MemoryStream stream;
  static constexpr size_t kBufferLength = 1;
  static constexpr size_t kBufferCount = 4;
  for (size_t i = 0; i < kBufferCount; i++) {
    Buffer buffer;
    buffer.data = shared_ptr<uint8_t>(
        new uint8_t[kBufferLength],
        default_delete<uint8_t[]>());
    buffer.data.get()[0] = i;
    buffer.length = kBufferLength;
    stream.append(buffer);
  }

  static constexpr size_t kStreamLength = kBufferLength * kBufferCount;
  ASSERT_EQ(kStreamLength, stream.size());

  Buffer outputBuffer;
  outputBuffer.data = shared_ptr<uint8_t>(
      new uint8_t[kStreamLength],
      default_delete<uint8_t[]>());
  outputBuffer.length = kStreamLength;
  size_t readBytes = stream.read(0, kStreamLength, &outputBuffer);

  ASSERT_EQ(kStreamLength, readBytes);
  for (size_t i = 0; i < kStreamLength; i++) {
    ASSERT_EQ(i, outputBuffer.data.get()[i]);
  }
}

TEST(WhenSeveralBuffersAreAddedAndReadWithByteAt, ItReturnsTheCorrectValues) {
  MemoryStream stream;
  static constexpr size_t kBufferLength = 1;
  static constexpr size_t kBufferCount = 4;
  for (size_t i = 0; i < kBufferCount; i++) {
    Buffer buffer;
    buffer.data = shared_ptr<uint8_t>(
        new uint8_t[kBufferLength],
        default_delete<uint8_t[]>());
    buffer.data.get()[0] = i;
    buffer.length = kBufferLength;
    stream.append(buffer);
  }

  static constexpr size_t kStreamLength = kBufferLength * kBufferCount;
  ASSERT_EQ(kStreamLength, stream.size());

  for (size_t i = 0; i < kStreamLength; i++) {
    ASSERT_EQ(i, stream.byteAt(i));
  }
}

TEST(WhenSeveralBuffersAreAdded, FirstIndexOfReturnsTheCorrectIndex) {
  MemoryStream stream;
  static constexpr size_t kBufferLength = 4;
  static constexpr size_t kBufferCount = 4;

  static constexpr size_t kStreamLength = kBufferLength * kBufferCount;
  const uint8_t bufferData[kStreamLength] = {
      static_cast<uint8_t>(0),
      static_cast<uint8_t>(1),
      static_cast<uint8_t>(2),
      static_cast<uint8_t>(3),
      static_cast<uint8_t>(4),
      static_cast<uint8_t>(5),
      static_cast<uint8_t>(6),
      static_cast<uint8_t>(7),
      static_cast<uint8_t>(0),
      static_cast<uint8_t>(1),
      static_cast<uint8_t>(2),
      static_cast<uint8_t>(3),
      static_cast<uint8_t>(4),
      static_cast<uint8_t>(5),
      static_cast<uint8_t>(6),
      static_cast<uint8_t>(7),
  };

  for (size_t i = 0; i < kBufferCount; i++) {
    Buffer buffer;
    buffer.data = shared_ptr<uint8_t>(
        new uint8_t[kBufferLength],
        default_delete<uint8_t[]>());

    for (size_t j = 0; j < kBufferLength; j++) {
      buffer.data.get()[j] = bufferData[i * kBufferLength + j];
    }

    buffer.length = kBufferLength;
    stream.append(buffer);
  }

  ASSERT_EQ(kStreamLength, stream.size());

  ASSERT_EQ(1, stream.firstIndexOf(1));
  ASSERT_EQ(9, stream.firstIndexOf(1, 2));
}

TEST(WhenSeveralBuffersAreAdded, FirstIndexOfReturnsTheCorrectIndexOfABuffer) {
  MemoryStream stream;
  static constexpr size_t kBufferLength = 4;
  static constexpr size_t kBufferCount = 4;

  static constexpr size_t kStreamLength = kBufferLength * kBufferCount;
  const uint8_t bufferData[kStreamLength] = {
      static_cast<uint8_t>(0),
      static_cast<uint8_t>(1),
      static_cast<uint8_t>(2),
      static_cast<uint8_t>(3),
      static_cast<uint8_t>(4),
      static_cast<uint8_t>(5),
      static_cast<uint8_t>(6),
      static_cast<uint8_t>(7),
      static_cast<uint8_t>(0),
      static_cast<uint8_t>(1),
      static_cast<uint8_t>(2),
      static_cast<uint8_t>(3),
      static_cast<uint8_t>(4),
      static_cast<uint8_t>(5),
      static_cast<uint8_t>(6),
      static_cast<uint8_t>(7),
  };

  for (size_t i = 0; i < kBufferCount; i++) {
    Buffer buffer;
    buffer.data = shared_ptr<uint8_t>(
        new uint8_t[kBufferLength],
        default_delete<uint8_t[]>());

    for (size_t j = 0; j < kBufferLength; j++) {
      buffer.data.get()[j] = bufferData[i * kBufferLength + j];
    }

    buffer.length = kBufferLength;
    stream.append(buffer);
  }

  ASSERT_EQ(kStreamLength, stream.size());

  uint8_t findBytes[] = {
      static_cast<uint8_t>(1),
      static_cast<uint8_t>(2),
  };

  ASSERT_EQ(1, stream.firstIndexOf(findBytes, sizeof(findBytes)));
  ASSERT_EQ(9, stream.firstIndexOf(findBytes, sizeof(findBytes), 2));

  findBytes[0] = kBufferLength - 1;
  findBytes[1] = kBufferLength;

  ASSERT_EQ(
      kBufferLength - 1,
      stream.firstIndexOf(findBytes, sizeof(findBytes)));
  ASSERT_EQ(
      MemoryStream::kNotFound,
      stream.firstIndexOf(findBytes, sizeof(findBytes), 12));
}

static Buffer stringToBuffer(const string& str) {
  const auto rawBuffer = shared_ptr<uint8_t>(new uint8_t[str.length()], default_delete<uint8_t[]>());
  Buffer buffer(rawBuffer, str.length());

  str.copy(reinterpret_cast<char*>(rawBuffer.get()), str.length());

  return buffer;
}

TEST(WhenSeveralBuffersAreAdded, FirstIndexOfReturnsTheCorrectIndexOfACString) {
  MemoryStream stream;

  Buffer buffer1("abcdefg");
  Buffer buffer2("hijklmnop");
  Buffer buffer3("qrstuvwxyz");

  stream.append(buffer1);
  stream.append(buffer2);
  stream.append(buffer3);

  ASSERT_EQ(26, stream.size());

  ASSERT_EQ(0, stream.firstIndexOf("a"));
  ASSERT_EQ(0, stream.firstIndexOf("abc"));
  ASSERT_EQ(0, stream.firstIndexOf("abcdefgh"));
  ASSERT_EQ(0, stream.firstIndexOf("abcdefghijklmnopq"));
  ASSERT_EQ(0, stream.firstIndexOf("abcdefghijklmnopqrstuvwxyz"));

  ASSERT_EQ(5, stream.firstIndexOf("fg"));

  ASSERT_EQ(6, stream.firstIndexOf("g"));
  ASSERT_EQ(6, stream.firstIndexOf("gh"));
  ASSERT_EQ(6, stream.firstIndexOf("ghijklmnopq"));
  ASSERT_EQ(6, stream.firstIndexOf("ghijklmnopqrstuvwxyz"));

  ASSERT_EQ(14, stream.firstIndexOf("op"));

  ASSERT_EQ(15, stream.firstIndexOf("p"));
  ASSERT_EQ(15, stream.firstIndexOf("pq"));
  ASSERT_EQ(15, stream.firstIndexOf("pqrstuvwxyz"));

  ASSERT_EQ(24, stream.firstIndexOf("yz"));
  ASSERT_EQ(25, stream.firstIndexOf("z"));
}

TEST(WhenSeveralBuffersAreAdded, FirstIndexOfReturnsTheCorrectIndexOfAStdString) {
  MemoryStream stream;

  Buffer buffer1("abcdefg");
  Buffer buffer2("hijklmnop");
  Buffer buffer3("qrstuvwxyz");

  stream.append(buffer1);
  stream.append(buffer2);
  stream.append(buffer3);

  ASSERT_EQ(26, stream.size());

  ASSERT_EQ(0, stream.firstIndexOf(string("a")));
  ASSERT_EQ(0, stream.firstIndexOf(string("abc")));
  ASSERT_EQ(0, stream.firstIndexOf(string("abcdefgh")));
  ASSERT_EQ(0, stream.firstIndexOf(string("abcdefghijklmnopq")));
  ASSERT_EQ(0, stream.firstIndexOf(string("abcdefghijklmnopqrstuvwxyz")));

  ASSERT_EQ(5, stream.firstIndexOf(string("fg")));

  ASSERT_EQ(6, stream.firstIndexOf(string("g")));
  ASSERT_EQ(6, stream.firstIndexOf(string("gh")));
  ASSERT_EQ(6, stream.firstIndexOf(string("ghijklmnopq")));
  ASSERT_EQ(6, stream.firstIndexOf(string("ghijklmnopqrstuvwxyz")));

  ASSERT_EQ(14, stream.firstIndexOf(string("op")));

  ASSERT_EQ(15, stream.firstIndexOf(string("p")));
  ASSERT_EQ(15, stream.firstIndexOf(string("pq")));
  ASSERT_EQ(15, stream.firstIndexOf(string("pqrstuvwxyz")));

  ASSERT_EQ(24, stream.firstIndexOf(string("yz")));
  ASSERT_EQ(25, stream.firstIndexOf(string("z")));
}

TEST(WhenSeveralBuffersAreAdded, FirstIndexOfAnyInSetReturnsTheCorrectIndex) {
  MemoryStream stream;
  static constexpr size_t kBufferLength = 4;
  static constexpr size_t kBufferCount = 4;

  static constexpr size_t kStreamLength = kBufferLength * kBufferCount;
  const uint8_t bufferData[kStreamLength] = {
      static_cast<uint8_t>(0),
      static_cast<uint8_t>(1),
      static_cast<uint8_t>(2),
      static_cast<uint8_t>(3),
      static_cast<uint8_t>(4),
      static_cast<uint8_t>(5),
      static_cast<uint8_t>(6),
      static_cast<uint8_t>(7),
      static_cast<uint8_t>(0),
      static_cast<uint8_t>(1),
      static_cast<uint8_t>(2),
      static_cast<uint8_t>(3),
      static_cast<uint8_t>(4),
      static_cast<uint8_t>(5),
      static_cast<uint8_t>(6),
      static_cast<uint8_t>(7),
  };

  for (size_t i = 0; i < kBufferCount; i++) {
    Buffer buffer;
    buffer.data = shared_ptr<uint8_t>(
        new uint8_t[kBufferLength],
        default_delete<uint8_t[]>());

    for (size_t j = 0; j < kBufferLength; j++) {
      buffer.data.get()[j] = bufferData[i * kBufferLength + j];
    }

    buffer.length = kBufferLength;
    stream.append(buffer);
  }

  ASSERT_EQ(kStreamLength, stream.size());

  {
    uint8_t data[] = {
        static_cast<uint8_t>(8),
        static_cast<uint8_t>(9),
        static_cast<uint8_t>(10),
        static_cast<uint8_t>(11),
        static_cast<uint8_t>(12),
    };

    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.firstIndexOfAnyInSet(data, sizeof(data)));
  }

  {
    uint8_t data[] = {
        static_cast<uint8_t>(3),
        static_cast<uint8_t>(4),
        static_cast<uint8_t>(5),
        static_cast<uint8_t>(6),
        static_cast<uint8_t>(7),
    };

    ASSERT_EQ(3, stream.firstIndexOfAnyInSet(data, sizeof(data)));
    ASSERT_EQ(11, stream.firstIndexOfAnyInSet(data, sizeof(data), 8));
    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.firstIndexOfAnyInSet(data, sizeof(data), 0, 3));
    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.firstIndexOfAnyInSet(data, sizeof(data), 8, 11));
  }

  {
    uint8_t data[] = {
        static_cast<uint8_t>(0),
        static_cast<uint8_t>(1),
        static_cast<uint8_t>(2),
        static_cast<uint8_t>(3),
        static_cast<uint8_t>(4),
    };
    ASSERT_EQ(0, stream.firstIndexOfAnyInSet(data, sizeof(data)));
    ASSERT_EQ(8, stream.firstIndexOfAnyInSet(data, sizeof(data), 5));
    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.firstIndexOfAnyInSet(data, sizeof(data), 5, 8));
    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.firstIndexOfAnyInSet(data, sizeof(data), 13, 16));
  }
}

TEST(WhenSeveralBuffersAreAdded, LastIndexOfAnyInSetReturnsTheCorrectIndex) {
  MemoryStream stream;
  static constexpr size_t kBufferLength = 4;
  static constexpr size_t kBufferCount = 4;
  static constexpr size_t kStreamLength = kBufferLength * kBufferCount;
  const uint8_t bufferData[kStreamLength] = {
      static_cast<uint8_t>(0),
      static_cast<uint8_t>(1),
      static_cast<uint8_t>(2),
      static_cast<uint8_t>(3),
      static_cast<uint8_t>(4),
      static_cast<uint8_t>(5),
      static_cast<uint8_t>(6),
      static_cast<uint8_t>(7),
      static_cast<uint8_t>(0),
      static_cast<uint8_t>(1),
      static_cast<uint8_t>(2),
      static_cast<uint8_t>(3),
      static_cast<uint8_t>(4),
      static_cast<uint8_t>(5),
      static_cast<uint8_t>(6),
      static_cast<uint8_t>(7),
  };

  for (size_t i = 0; i < kBufferCount; i++) {
    Buffer buffer;
    buffer.data = shared_ptr<uint8_t>(
        new uint8_t[kBufferLength],
        default_delete<uint8_t[]>());

    for (size_t j = 0; j < kBufferLength; j++) {
      buffer.data.get()[j] = bufferData[i * kBufferLength + j];
    }

    buffer.length = kBufferLength;
    stream.append(buffer);
  }

  ASSERT_EQ(kStreamLength, stream.size());

  {
    uint8_t data[] = {
        static_cast<uint8_t>(8),
        static_cast<uint8_t>(9),
        static_cast<uint8_t>(10),
        static_cast<uint8_t>(11),
        static_cast<uint8_t>(12),
    };

    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.lastIndexOfAnyInSet(data, sizeof(data)));
  }

  {
    uint8_t data[] = {
        static_cast<uint8_t>(7),
        static_cast<uint8_t>(6),
        static_cast<uint8_t>(5),
        static_cast<uint8_t>(4),
        static_cast<uint8_t>(3),
    };

    ASSERT_EQ(15, stream.lastIndexOfAnyInSet(data, sizeof(data)));
    ASSERT_EQ(7, stream.lastIndexOfAnyInSet(data, sizeof(data), 1, 8));
    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.lastIndexOfAnyInSet(data, sizeof(data), 0, 3));
    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.lastIndexOfAnyInSet(data, sizeof(data), 8, 11));
  }

  {
    uint8_t data[] = {
        static_cast<uint8_t>(0),
        static_cast<uint8_t>(1),
        static_cast<uint8_t>(2),
        static_cast<uint8_t>(3),
        static_cast<uint8_t>(4),
    };

    ASSERT_EQ(12, stream.lastIndexOfAnyInSet(data, sizeof(data)));
    ASSERT_EQ(4, stream.lastIndexOfAnyInSet(data, sizeof(data), 1, 8));
    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.lastIndexOfAnyInSet(data, sizeof(data), 5, 8));
    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.lastIndexOfAnyInSet(data, sizeof(data), 13, 16));
  }
}

TEST(
    WhenSeveralBuffersAreAdded,
    FirstIndexNotOfAnyInSetReturnsTheCorrectIndex) {
  MemoryStream stream;
  static constexpr size_t kBufferLength = 4;
  static constexpr size_t kBufferCount = 4;
  static constexpr size_t kStreamLength = kBufferLength * kBufferCount;
  const uint8_t bufferData[kStreamLength] = {
      static_cast<uint8_t>(0),
      static_cast<uint8_t>(1),
      static_cast<uint8_t>(2),
      static_cast<uint8_t>(3),
      static_cast<uint8_t>(4),
      static_cast<uint8_t>(5),
      static_cast<uint8_t>(6),
      static_cast<uint8_t>(7),
      static_cast<uint8_t>(0),
      static_cast<uint8_t>(1),
      static_cast<uint8_t>(2),
      static_cast<uint8_t>(3),
      static_cast<uint8_t>(4),
      static_cast<uint8_t>(5),
      static_cast<uint8_t>(6),
      static_cast<uint8_t>(7),
  };

  for (size_t i = 0; i < kBufferCount; i++) {
    Buffer buffer;
    buffer.data = shared_ptr<uint8_t>(
        new uint8_t[kBufferLength],
        default_delete<uint8_t[]>());

    for (size_t j = 0; j < kBufferLength; j++) {
      buffer.data.get()[j] = bufferData[i * kBufferLength + j];
    }

    buffer.length = kBufferLength;
    stream.append(buffer);
  }

  ASSERT_EQ(kStreamLength, stream.size());

  {
    uint8_t data[] = {
        static_cast<uint8_t>(8),
        static_cast<uint8_t>(9),
        static_cast<uint8_t>(10),
        static_cast<uint8_t>(11),
        static_cast<uint8_t>(12),
    };

    ASSERT_EQ(0, stream.firstIndexNotOfAnyInSet(data, sizeof(data)));
    ASSERT_EQ(8, stream.firstIndexNotOfAnyInSet(data, sizeof(data), 8));
  }

  {
    uint8_t data[] = {
        static_cast<uint8_t>(0),
        static_cast<uint8_t>(1),
        static_cast<uint8_t>(2),
        static_cast<uint8_t>(3),
        static_cast<uint8_t>(4),
        static_cast<uint8_t>(5),
        static_cast<uint8_t>(6),
        static_cast<uint8_t>(7),
    };

    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.firstIndexNotOfAnyInSet(data, sizeof(data)));
  }

  {
    uint8_t data[] = {
        static_cast<uint8_t>(7),
        static_cast<uint8_t>(6),
        static_cast<uint8_t>(5),
        static_cast<uint8_t>(4),
        static_cast<uint8_t>(3),
    };

    ASSERT_EQ(0, stream.firstIndexNotOfAnyInSet(data, sizeof(data)));
    ASSERT_EQ(8, stream.firstIndexNotOfAnyInSet(data, sizeof(data), 8));
    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.firstIndexNotOfAnyInSet(data, sizeof(data), 3, 7));
    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.firstIndexNotOfAnyInSet(data, sizeof(data), 11, 15));
  }

  {
    uint8_t data[] = {
        static_cast<uint8_t>(4),
        static_cast<uint8_t>(3),
        static_cast<uint8_t>(2),
        static_cast<uint8_t>(1),
        static_cast<uint8_t>(0),
    };

    ASSERT_EQ(5, stream.firstIndexNotOfAnyInSet(data, sizeof(data)));
    ASSERT_EQ(13, stream.firstIndexNotOfAnyInSet(data, sizeof(data), 8));
    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.firstIndexNotOfAnyInSet(data, sizeof(data), 0, 4));
    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.firstIndexNotOfAnyInSet(data, sizeof(data), 8, 12));
  }
}

TEST(WhenSeveralBuffersAreAdded, LastIndexNotOfAnyInSetReturnsTheCorrectIndex) {
  MemoryStream stream;
  static constexpr size_t kBufferLength = 4;
  static constexpr size_t kBufferCount = 4;
  static constexpr size_t kStreamLength = kBufferLength * kBufferCount;
  const uint8_t bufferData[kStreamLength] = {
      static_cast<uint8_t>(0),
      static_cast<uint8_t>(1),
      static_cast<uint8_t>(2),
      static_cast<uint8_t>(3),
      static_cast<uint8_t>(4),
      static_cast<uint8_t>(5),
      static_cast<uint8_t>(6),
      static_cast<uint8_t>(7),
      static_cast<uint8_t>(0),
      static_cast<uint8_t>(1),
      static_cast<uint8_t>(2),
      static_cast<uint8_t>(3),
      static_cast<uint8_t>(4),
      static_cast<uint8_t>(5),
      static_cast<uint8_t>(6),
      static_cast<uint8_t>(7),
  };

  for (size_t i = 0; i < kBufferCount; i++) {
    Buffer buffer;
    buffer.data = shared_ptr<uint8_t>(
        new uint8_t[kBufferLength],
        default_delete<uint8_t[]>());

    for (size_t j = 0; j < kBufferLength; j++) {
      buffer.data.get()[j] = bufferData[i * kBufferLength + j];
    }

    buffer.length = kBufferLength;
    stream.append(buffer);
  }

  ASSERT_EQ(kStreamLength, stream.size());

  {
    uint8_t data[] = {
        static_cast<uint8_t>(8),
        static_cast<uint8_t>(9),
        static_cast<uint8_t>(10),
        static_cast<uint8_t>(11),
        static_cast<uint8_t>(12),
    };

    ASSERT_EQ(0, stream.firstIndexNotOfAnyInSet(data, sizeof(data)));
    ASSERT_EQ(8, stream.firstIndexNotOfAnyInSet(data, sizeof(data), 8));
  }

  {
    uint8_t data[] = {
        static_cast<uint8_t>(0),
        static_cast<uint8_t>(1),
        static_cast<uint8_t>(2),
        static_cast<uint8_t>(3),
        static_cast<uint8_t>(4),
        static_cast<uint8_t>(5),
        static_cast<uint8_t>(6),
        static_cast<uint8_t>(7),
    };

    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.lastIndexNotOfAnyInSet(data, sizeof(data)));
  }

  {
    uint8_t data[] = {
        static_cast<uint8_t>(7),
        static_cast<uint8_t>(6),
        static_cast<uint8_t>(5),
        static_cast<uint8_t>(4),
        static_cast<uint8_t>(3),
    };

    ASSERT_EQ(10, stream.lastIndexNotOfAnyInSet(data, sizeof(data)));
    ASSERT_EQ(2, stream.lastIndexNotOfAnyInSet(data, sizeof(data), 1, 8));
    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.lastIndexNotOfAnyInSet(data, sizeof(data), 3, 8));
    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.lastIndexNotOfAnyInSet(data, sizeof(data), 11, 16));
  }

  {
    uint8_t data[] = {
        static_cast<uint8_t>(0),
        static_cast<uint8_t>(1),
        static_cast<uint8_t>(2),
        static_cast<uint8_t>(3),
        static_cast<uint8_t>(4),
    };

    ASSERT_EQ(15, stream.lastIndexNotOfAnyInSet(data, sizeof(data)));
    ASSERT_EQ(7, stream.lastIndexNotOfAnyInSet(data, sizeof(data), 1, 8));
    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.lastIndexNotOfAnyInSet(data, sizeof(data), 0, 4));
    ASSERT_EQ(
        MemoryStream::kNotFound,
        stream.lastIndexNotOfAnyInSet(data, sizeof(data), 8, 12));
  }
}

TEST(WhenSeveralBuffersAreAdded, SubstreamsContainTheCorrectBytes) {
  MemoryStream stream;
  static constexpr size_t kBufferLength = 4;
  static constexpr size_t kBufferCount = 4;
  static constexpr size_t kStreamLength = kBufferLength * kBufferCount;
  const uint8_t bufferData[kStreamLength] = {
      static_cast<uint8_t>(0),
      static_cast<uint8_t>(1),
      static_cast<uint8_t>(2),
      static_cast<uint8_t>(3),
      static_cast<uint8_t>(4),
      static_cast<uint8_t>(5),
      static_cast<uint8_t>(6),
      static_cast<uint8_t>(7),
      static_cast<uint8_t>(0),
      static_cast<uint8_t>(1),
      static_cast<uint8_t>(2),
      static_cast<uint8_t>(3),
      static_cast<uint8_t>(4),
      static_cast<uint8_t>(5),
      static_cast<uint8_t>(6),
      static_cast<uint8_t>(7),
  };

  for (size_t i = 0; i < kBufferCount; i++) {
    Buffer buffer;
    buffer.data = shared_ptr<uint8_t>(
        new uint8_t[kBufferLength],
        default_delete<uint8_t[]>());

    for (size_t j = 0; j < kBufferLength; j++) {
      buffer.data.get()[j] = bufferData[i * kBufferLength + j];
    }

    buffer.length = kBufferLength;
    stream.append(buffer);
  }

  ASSERT_EQ(kStreamLength, stream.size());

  static constexpr size_t kSubstreamOffset = 5;
  static constexpr size_t kSubstreamEndOffset = 1;
  static constexpr size_t kSubstreamLength =
      kStreamLength - kSubstreamOffset - kSubstreamEndOffset;

  auto substream =
      stream.subStream(kSubstreamOffset, kStreamLength - kSubstreamEndOffset);
  ASSERT_EQ(kSubstreamLength, substream.size());

  for (size_t i = 0; i < kSubstreamLength; i++) {
    ASSERT_EQ(bufferData[i + kSubstreamOffset], substream.byteAt(i));
  }
}

TEST(WhenSeveralBuffersAreAdded, EqualsWorks) {
  MemoryStream stream;
  static constexpr size_t kBufferLength = 4;
  static constexpr size_t kBufferCount = 4;
  static constexpr size_t kStreamLength = kBufferLength * kBufferCount;
  const uint8_t bufferData[kStreamLength] = {
      static_cast<uint8_t>(0),
      static_cast<uint8_t>(1),
      static_cast<uint8_t>(2),
      static_cast<uint8_t>(3),
      static_cast<uint8_t>(4),
      static_cast<uint8_t>(5),
      static_cast<uint8_t>(6),
      static_cast<uint8_t>(7),
      static_cast<uint8_t>(0),
      static_cast<uint8_t>(1),
      static_cast<uint8_t>(2),
      static_cast<uint8_t>(3),
      static_cast<uint8_t>(4),
      static_cast<uint8_t>(5),
      static_cast<uint8_t>(6),
      static_cast<uint8_t>(7),
  };

  for (size_t i = 0; i < kBufferCount; i++) {
    Buffer buffer;
    buffer.data = shared_ptr<uint8_t>(
        new uint8_t[kBufferLength],
        default_delete<uint8_t[]>());

    for (size_t j = 0; j < kBufferLength; j++) {
      buffer.data.get()[j] = bufferData[i * kBufferLength + j];
    }

    buffer.length = kBufferLength;
    stream.append(buffer);
  }

  ASSERT_EQ(kStreamLength, stream.size());

  const uint8_t expectedBytes[] = {
      static_cast<uint8_t>(6),
      static_cast<uint8_t>(7),
      static_cast<uint8_t>(0),
  };

  ASSERT_TRUE(stream.equals(expectedBytes, sizeof(expectedBytes), 6));
}

TEST(WhenSeveralBuffersAreAdded, SplitWorks) {
  MemoryStream stream;

  Buffer buffer;
  char stringToSplit[] = "Split this by spaces.";

  buffer.data = shared_ptr<uint8_t>(
      reinterpret_cast<uint8_t*>(stringToSplit),
      [](uint8_t*) {});

  buffer.length = strlen(stringToSplit);
  stream.append(buffer);

  stream.split(' ', [](size_t index, MemoryStream&& stream) {
    switch (index) {
      case 0:
        EXPECT_TRUE(stream.equalsString("Split"));
        break;

      case 1:
        EXPECT_TRUE(stream.equalsString("this"));
        break;

      case 2:
        EXPECT_TRUE(stream.equalsString("by"));
        break;

      case 3:
        EXPECT_TRUE(stream.equalsString("spaces."));
        break;

      default:
        ADD_FAILURE() << "Unexpected index " << index;
    }

    return true;
  });
}

TEST(WhenStreamHasLeadingAndTrailingSeparators, SplitWorks) {
  MemoryStream stream;

  Buffer buffer;
  char stringToSplit[] = " Split this by spaces. ";

  buffer.data = shared_ptr<uint8_t>(
      reinterpret_cast<uint8_t*>(stringToSplit),
      [](uint8_t*) {});

  buffer.length = strlen(stringToSplit);
  stream.append(buffer);

  stream.split(' ', [](size_t index, MemoryStream&& stream) {
    switch (index) {
      case 0:
        EXPECT_TRUE(stream.equalsString(""));
        break;

      case 1:
        EXPECT_TRUE(stream.equalsString("Split"));
        break;

      case 2:
        EXPECT_TRUE(stream.equalsString("this"));
        break;

      case 3:
        EXPECT_TRUE(stream.equalsString("by"));
        break;

      case 4:
        EXPECT_TRUE(stream.equalsString("spaces."));

      case 5:
        EXPECT_TRUE(stream.equalsString(""));
        break;

      default:
        ADD_FAILURE() << "Unexpected index " << index;
    }

    return true;
  });
}


TEST(WhenStreamHasLeadingAndTrailingSeparators, SplitIntoStringsWorks) {
  MemoryStream stream;

  Buffer buffer;
  char stringToSplit[] = " Split this by spaces. ";

  buffer.data = shared_ptr<uint8_t>(
      reinterpret_cast<uint8_t*>(stringToSplit),
      [](uint8_t*) {});

  buffer.length = strlen(stringToSplit);
  stream.append(buffer);

  const vector<string> strings = stream.splitIntoStrings(' ');

  ASSERT_EQ(6, strings.size());
  ASSERT_STREQ("", strings[0].c_str());
  ASSERT_STREQ("Split", strings[1].c_str());
  ASSERT_STREQ("this", strings[2].c_str());
  ASSERT_STREQ("by", strings[3].c_str());
  ASSERT_STREQ("spaces.", strings[4].c_str());
  ASSERT_STREQ("", strings[5].c_str());
}

TEST(WhenStreamHasLeadingAndTrailingSeparators, SplitIntoStringsWithMaxTokenCountWorks) {
  MemoryStream stream;

  Buffer buffer;
  char stringToSplit[] = " Split this by spaces. ";

  buffer.data = shared_ptr<uint8_t>(
      reinterpret_cast<uint8_t*>(stringToSplit),
      [](uint8_t*) {});

  buffer.length = strlen(stringToSplit);
  stream.append(buffer);

  const vector<string> strings = stream.splitIntoStrings(' ', 3);

  ASSERT_EQ(3, strings.size());
  ASSERT_STREQ("", strings[0].c_str());
  ASSERT_STREQ("Split", strings[1].c_str());
  ASSERT_STREQ("this by spaces. ", strings[2].c_str());
}

TEST(WhenStreamHasLeadingAndTrailingWhitespace, TrimWorks) {
  MemoryStream stream;

  Buffer buffer;
  char trimMe[] = " trimMe\r\n";

  buffer.data =
      shared_ptr<uint8_t>(reinterpret_cast<uint8_t*>(trimMe), [](uint8_t*) {});

  buffer.length = strlen(trimMe);
  stream.append(buffer);

  ASSERT_TRUE(stream.trim().equalsString("trimMe"))
      << "'" << stream.trim() << "'";
}

TEST(WhenCleared, NoDataIsLeft) {
  MemoryStream stream;
  Buffer buffer;
  buffer.data =
      shared_ptr<uint8_t>(new uint8_t[1], default_delete<uint8_t[]>());
  buffer.length = 1;
  stream.append(buffer);

  stream.clear();
  ASSERT_EQ(0, stream.size());

  stream.visitBuffers([](size_t index, Buffer&& buffer) {
    ADD_FAILURE();
    return false;
  });
}

}  // namespace maplang
