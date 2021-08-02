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

#include "nodes/ParameterRouter.h"

using namespace std;
using namespace nlohmann;

namespace maplang {

const string ParameterRouter::kInitParameter_RoutingKey = "routingKey";

ParameterRouter::ParameterRouter(const json& initParameters) {
  if (!initParameters.contains(kInitParameter_RoutingKey)) {
    throw runtime_error(
        "Parameter Router requires field '" + kInitParameter_RoutingKey + "'");
  }

  const json& key = initParameters[kInitParameter_RoutingKey];
  if (!key.is_string()) {
    throw runtime_error(
        "Field '" + kInitParameter_RoutingKey
        + "' be a string in Parameter Router");
  }

  mRoutingKey = json_pointer<basic_json<>>(
      initParameters[kInitParameter_RoutingKey].get<string>());
}

void ParameterRouter::handlePacket(const PathablePacket& pathablePacket) {
  const json& parameters = pathablePacket.packet.parameters;

  if (!parameters.contains(mRoutingKey)) {
    throw runtime_error(
        "Packet must contain key '" + mRoutingKey.to_string()
        + "' in Parameter Router");
  }

  const json& channelValue = parameters[mRoutingKey];

  if (!channelValue.is_string() && !channelValue.is_primitive()) {
    throw runtime_error(
        "Value of '" + mRoutingKey.to_string() + "' must be a simple type.");
  }

  const string channel = channelValue.get<string>();

  pathablePacket.packetPusher->pushPacket(pathablePacket.packet, channel);
}

}  // namespace maplang
