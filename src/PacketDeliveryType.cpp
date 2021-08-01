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

#include "maplang/PacketDeliveryType.h"

#include <string>

using namespace std;

namespace maplang {

void to_json(nlohmann::json& j, const PacketDeliveryType& packetDelivery) {
  switch (packetDelivery) {
    case PacketDeliveryType::PushDirectlyToTarget:
      j = "Push Directly To Target";
      break;

    case PacketDeliveryType::AlwaysQueue:
      j = "Always Queue";
      break;

    default:
      throw invalid_argument(
          "Unknown packet delivery type: "
              + to_string(static_cast<uint32_t>(packetDelivery)));
  }
}

void from_json(const nlohmann::json& j, PacketDeliveryType& packetDelivery) {
  const string str = j.get<string>();

  if (str == "Push Directly To Target") {
    packetDelivery = PacketDeliveryType::PushDirectlyToTarget;
  } else if (str == "Always Queue") {
    packetDelivery = PacketDeliveryType::AlwaysQueue;
  } else {
    throw invalid_argument("Unknown PacketDeliveryType '" + str + "'.");
  }
}

}  // namespace maplang
