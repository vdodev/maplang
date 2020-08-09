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

#include "PacketReader.h"
#include <sstream>

using namespace std;
using namespace nlohmann;

namespace maplang {

static uint64_t readUInt64BE(const uint8_t** where) {
  uint64_t val = 0;
  const uint8_t* startFrom = *where;
  val = static_cast<uint64_t>(startFrom[0]) << 56
      | static_cast<uint64_t>(startFrom[1]) << 48
      | static_cast<uint64_t>(startFrom[2]) << 40
      | static_cast<uint64_t>(startFrom[3]) << 32
      | static_cast<uint64_t>(startFrom[4]) << 24
      | static_cast<uint64_t>(startFrom[5]) << 16
      | static_cast<uint64_t>(startFrom[6]) <<  8
      | static_cast<uint64_t>(startFrom[7]);

  *where += sizeof(uint64_t);

  return val;
}

void PacketReader::handlePacket(const PathablePacket* incomingPacket) {
  mPendingBytes.append(incomingPacket->buffers[0]);

  while (mPendingBytes.size() > 0) {
    if (mLength == 0 && mPendingBytes.size() >= sizeof(uint64_t)) {
      uint64_t followingLengthBigEndian;
      mPendingBytes.read(
          0,
          sizeof(followingLengthBigEndian),
          &followingLengthBigEndian,
          sizeof(followingLengthBigEndian));

      mLength = sizeof(followingLengthBigEndian) + be64toh(followingLengthBigEndian);
    }

    if (mPendingBytes.size() >= mLength) {
      MemoryStream deserializeStream = mPendingBytes.subStream(sizeof(uint64_t), sizeof(uint64_t) + mLength);

      try {
        Packet packet = readPacket(deserializeStream);
        mPendingBytes = mPendingBytes.subStream(mLength);
        mLength = 0;

        incomingPacket->packetPusher->pushPacket(&packet, "Packet Ready");
      } catch (exception &e) {
        Packet errorPacket;
        errorPacket.parameters["errorMessage"] = e.what();

        incomingPacket->packetPusher->pushPacket(&errorPacket, "error");
      }
    }
  }
}

Packet PacketReader::readPacket(const MemoryStream& stream) {
  uint64_t offset = sizeof(uint64_t);

  uint64_t parametersLengthBigEndian;
  stream.read(
      offset,
      sizeof(parametersLengthBigEndian),
      &parametersLengthBigEndian,
      sizeof(parametersLengthBigEndian));
  uint64_t parametersLength = be64toh(parametersLengthBigEndian);
  offset += parametersLength;

  auto parameterStream = stream.subStream(sizeof(parametersLengthBigEndian));
  Buffer parameterBuffer;
  parameterBuffer.length = parametersLength;
  parameterBuffer.data = shared_ptr<uint8_t>(new uint8_t[parametersLength + 1], default_delete<uint8_t[]>());
  parameterStream.read(2 * sizeof(uint64_t), parametersLength, &parameterBuffer);

  Packet parsedPacket;
  parsedPacket.parameters = json::from_msgpack(parameterBuffer.data.get());
  parameterBuffer.data.reset();
  parameterBuffer.length = 0;

  while (offset < stream.size()) {
    uint64_t bufferSizeBigEndian;
    stream.read(offset, sizeof(bufferSizeBigEndian), &bufferSizeBigEndian, sizeof(bufferSizeBigEndian));
    const uint64_t bufferSize = be64toh(bufferSizeBigEndian);
    offset += sizeof(bufferSizeBigEndian);

    Buffer buffer;
    buffer.length = bufferSize;
    buffer.data = shared_ptr<uint8_t>(new uint8_t[bufferSize], default_delete<uint8_t[]>());
    size_t bytesRead = stream.read(offset, bufferSize, buffer.data.get(), bufferSize);
    if (bytesRead != bufferSize) {
      throw runtime_error("Failed to parse data. Buffer length is longer than buffer.");
    }

    parsedPacket.buffers.push_back(buffer);
    offset += bufferSize;
  }

  return parsedPacket;
}

}  // namespace maplang
