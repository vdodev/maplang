/*
 * Copyright 2020 VDO Dev Inc <support@maplang.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "nodes/OrderedPacketSender.h"

using namespace std;

static const string kChannel_First = "first";
static const string kChannel_Last = "last";

namespace maplang {

void OrderedPacketSender::handlePacket(const PathablePacket& incomingPacket) {
  Packet p;
  p.parameters = incomingPacket.parameters;
  p.buffers = incomingPacket.buffers;

  incomingPacket.packetPusher->pushPacket(p, kChannel_First);
  incomingPacket.packetPusher->pushPacket(move(p), kChannel_Last);
}

}  // namespace maplang
