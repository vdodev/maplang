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

#include "nodes/PacketReader.h"

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
        | static_cast<uint64_t>(startFrom[6]) << 8
        | static_cast<uint64_t>(startFrom[7]);

  *where += sizeof(uint64_t);

  return val;
}

PacketReader::PacketReader(const Factories& factories)
    : mFactories(factories) {}

void PacketReader::handlePacket(const PathablePacket& incomingPathablePacket) {
  const Packet& incomingPacket = incomingPathablePacket.packet;
  mPendingBytes.append(incomingPacket.buffers[0]);

  while (mPendingBytes.size() > 0) {
    if (mLength == 0) {
      if (mPendingBytes.size() >= sizeof(uint64_t)) {
        const size_t readFromOffset = 0;
        uint64_t followingLength =
            mPendingBytes.readBigEndian<uint64_t>(readFromOffset);

        mLength = sizeof(followingLength) + followingLength;
      } else {
        return;
      }
    }

    if (mPendingBytes.size() >= mLength) {
      MemoryStream deserializeStream =
          mPendingBytes.subStream(sizeof(uint64_t), sizeof(uint64_t) + mLength);

      try {
        Packet packet = readPacket(deserializeStream);
        mPendingBytes = mPendingBytes.subStream(mLength);
        mLength = 0;

        incomingPathablePacket.packetPusher->pushPacket(
            move(packet),
            "Packet Ready");
      } catch (exception& e) {
        Packet errorPacket;
        errorPacket.parameters["errorMessage"] = e.what();

        incomingPathablePacket.packetPusher->pushPacket(
            move(errorPacket),
            "error");
      }
    }
  }
}

Packet PacketReader::readPacket(const MemoryStream& stream) const {
  uint64_t offset = sizeof(uint64_t);

  const uint64_t parametersLength = stream.readBigEndian<uint64_t>(offset);

  offset += parametersLength;

  auto parameterStream = stream.subStream(sizeof(parametersLength));
  Buffer parameterBuffer =
      mFactories.bufferFactory->Create(parametersLength + 1);

  parameterStream.read(
      2 * sizeof(uint64_t),
      parametersLength,
      &parameterBuffer);

  Packet parsedPacket;
  parsedPacket.parameters = json::from_msgpack(parameterBuffer.data.get());
  parameterBuffer.data.reset();
  parameterBuffer.length = 0;

  while (offset < stream.size()) {
    const uint64_t bufferSize = stream.readBigEndian<uint64_t>(offset);

    offset += sizeof(bufferSize);

    const Buffer buffer = mFactories.bufferFactory->Create(bufferSize);
    const size_t bytesRead =
        stream.read(offset, bufferSize, buffer.data.get(), bufferSize);

    if (bytesRead != bufferSize) {
      THROW(
          "Failed to parse data. Buffer length "
          << bufferSize << " is longer than available byte count " << bytesRead
          << ".");
    }

    parsedPacket.buffers.push_back(buffer);
    offset += bufferSize;
  }

  return parsedPacket;
}

}  // namespace maplang
