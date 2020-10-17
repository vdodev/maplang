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

#ifndef __MAPLANG_PATHABLEPACKET_H__
#define __MAPLANG_PATHABLEPACKET_H__

#include <memory>
#include <vector>

#include "maplang/Buffer.h"
#include "maplang/IPacketPusher.h"
#include "maplang/json.hpp"
#include "maplang/Packet.h"

namespace maplang {

class IPacketPusher;

struct PathablePacket {
  PathablePacket(const Packet& packet, const std::shared_ptr<IPacketPusher>& packetPusher);

  const Packet& packet;
  std::shared_ptr<IPacketPusher> packetPusher;
};

}  // namespace maplang

#endif  // __MAPLANG_PATHABLEPACKET_H__
