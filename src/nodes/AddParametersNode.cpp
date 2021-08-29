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

#include "nodes/AddParametersNode.h"

namespace maplang {

AddParametersNode::AddParametersNode(
    const Factories& factories,
    const nlohmann::json& initParams)
    : mFactories(factories), mParametersToAdd(initParams) {}

void AddParametersNode::handlePacket(const PathablePacket& incomingPacket) {
  Packet p = incomingPacket.packet;

  if (p.parameters == nullptr) {
    p.parameters = mParametersToAdd;
  } else {
    p.parameters.update(mParametersToAdd.begin(), mParametersToAdd.end());
  }

  incomingPacket.packetPusher->pushPacket(std::move(p), "Added Parameters");
}

}  // namespace maplang
