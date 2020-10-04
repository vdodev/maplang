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

#ifndef __MAPLANG_IPACKETPUSHER_H__
#define __MAPLANG_IPACKETPUSHER_H__

#include "maplang/Packet.h"

namespace maplang {
/**
 * Pushes packets into a DataGraph.
 */
class IPacketPusher {
 public:
  virtual ~IPacketPusher() {}

  /**
   * Pushes a packet into the Graph.
   *
   * This method is thread-safe, but if called from multiple threads
   * simultaneously, packet processing order is not guaranteed.
   *
   * If the implementation is also an ISink, this can safely be called from
   * ISink::handlePacket().
   *
   * @param packet Packet to push into the Graph
   * @param channelName Channel this packet is coming from.
   */
  virtual void pushPacket(const Packet& packet, const std::string& channelName) = 0;

  virtual void pushPacket(Packet&& packet, const std::string& channelName) = 0;
};

}  // namespace maplang

#endif  // __MAPLANG_IPACKETPUSHER_H__
