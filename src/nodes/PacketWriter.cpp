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

#include "nodes/PacketWriter.h"

#include <sstream>

using namespace std;
using namespace nlohmann;

namespace maplang {

static void writeUInt64BE(uint64_t val, uint8_t** where) {
  **where++ = val >> 56;
  **where++ = 0xFF & (val >> 48);
  **where++ = 0xFF & (val >> 40);
  **where++ = 0xFF & (val >> 32);
  **where++ = 0xFF & (val >> 24);
  **where++ = 0xFF & (val >> 16);
  **where++ = 0xFF & (val >> 8);
  **where++ = 0xFF & val;
}

void PacketWriter::handlePacket(const PathablePacket& incomingPathablePacket) {
  const Packet& incomingPacket = incomingPathablePacket.packet;
  basic_stringstream<uint8_t> parameterStream;
  json::to_msgpack(incomingPacket.parameters, parameterStream);

  parameterStream.seekg(0, ios::end);
  const size_t parametersLength = parameterStream.tellg();
  parameterStream.seekg(0, ios::beg);

  // number of bytes following the first length field
  size_t totalLength =
      parametersLength + (1 + incomingPacket.buffers.size()) * sizeof(uint64_t);

  for (size_t i = 0; i < incomingPacket.buffers.size(); i++) {
    totalLength += incomingPacket.buffers[i].length;
  }

  Buffer buffer;
  buffer.data = shared_ptr<uint8_t[]>(new uint8_t[totalLength]);
  buffer.length = totalLength;

  uint8_t* writeTo = buffer.data.get();
  writeUInt64BE(totalLength, &writeTo);
  writeUInt64BE(parametersLength, &writeTo);
  parameterStream.read(writeTo, parametersLength);
  writeTo += parametersLength;

  for (size_t i = 0; i < incomingPacket.buffers.size(); i++) {
    const size_t bufferLength = incomingPacket.buffers[i].length;
    writeUInt64BE(bufferLength, &writeTo);
    memcpy(writeTo, incomingPacket.buffers[i].data.get(), bufferLength);
    writeTo += bufferLength;
  }

  Packet sendPacket;
  sendPacket.buffers.push_back(buffer);
  incomingPathablePacket.packetPusher->pushPacket(
      move(sendPacket),
      "Message Ready");
}

}  // namespace maplang
