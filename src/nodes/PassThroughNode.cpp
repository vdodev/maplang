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

#include "nodes/PassThroughNode.h"

using namespace std;

namespace maplang {

const string PassThroughNode::kInputParam_OutputChannel = "outputChannel";

PassThroughNode::PassThroughNode(const nlohmann::json& initParameters)
    : mOutputChannel(
        initParameters[kInputParam_OutputChannel].get<std::string>()) {}

void PassThroughNode::handlePacket(const PathablePacket& incomingPacket) {
  incomingPacket.packetPusher->pushPacket(
      incomingPacket.packet,
      mOutputChannel);
}

}  // namespace maplang
