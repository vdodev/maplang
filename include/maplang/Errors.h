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

#ifndef MAPLANG__ERRORS_H_
#define MAPLANG__ERRORS_H_

#include <string>

#include "maplang/IPacketPusher.h"
#include "maplang/Packet.h"

namespace maplang {

const extern std::string kChannel_Error;

const extern std::string kParameter_ErrorName;
const extern std::string kParameter_ErrorMessage;

inline Packet createErrorPacket(
    const std::string& errorName,
    const std::string& errorMessage,
    const nlohmann::json& extraParameters = nullptr) {
  Packet packet;
  if (extraParameters != nullptr) {
    packet.parameters = extraParameters;
  }

  packet.parameters[kParameter_ErrorName] = errorName;
  packet.parameters[kParameter_ErrorMessage] = errorMessage;

  return packet;
}

inline void sendErrorPacket(
    const std::shared_ptr<IPacketPusher>& packetPusher,
    const std::string& errorName,
    const std::string& errorMessage,
    const nlohmann::json& extraParameters = nullptr) {
  Packet errorPacket = createErrorPacket(errorName, errorMessage, extraParameters);
  packetPusher->pushPacket(std::move(errorPacket), kChannel_Error);
}

inline void sendErrorPacket(
    const std::shared_ptr<IPacketPusher>& packetPusher,
    const std::exception& ex,
    const nlohmann::json& extraParameters = nullptr) {
  Packet errorPacket = createErrorPacket("exception", ex.what(), extraParameters);
  packetPusher->pushPacket(std::move(errorPacket), kChannel_Error);
}

}  // namespace maplang
#endif  // MAPLANG__ERRORS_H_
