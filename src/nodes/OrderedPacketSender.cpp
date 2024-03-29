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

OrderedPacketSender::OrderedPacketSender(
    const Factories& factories,
    const nlohmann::json& initParameters)
    : mFactories(factories), mInitParameteres(initParameters) {}

void OrderedPacketSender::handlePacket(
    const PathablePacket& incomingPathablePacket) {
  const Packet& incomingPacket = incomingPathablePacket.packet;
  incomingPathablePacket.packetPusher->pushPacket(
      incomingPacket,
      kChannel_First);
  incomingPathablePacket.packetPusher->pushPacket(
      incomingPacket,
      kChannel_Last);
}

}  // namespace maplang
