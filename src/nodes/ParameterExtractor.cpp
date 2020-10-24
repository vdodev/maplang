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

#include "nodes/ParameterExtractor.h"

#include <sstream>

using namespace std;
using namespace nlohmann;

namespace maplang {

const std::string ParameterExtractor::kChannel_ExtractedParameter =
    "Extracted Parameter";

ParameterExtractor::ParameterExtractor(const nlohmann::json& initParameters)
    : mParameterJsonPointerToExtract(json_pointer<basic_json<>>(
        initParameters["extractParameter"].get<string>())) {}

void ParameterExtractor::handlePacket(const PathablePacket& incomingPacket) {
  Packet packetWithExtractedParameter;

  const auto& incomingParams = incomingPacket.packet.parameters;
  if (!incomingParams.contains(mParameterJsonPointerToExtract)) {
    return;
  }

  packetWithExtractedParameter.parameters =
      incomingParams[mParameterJsonPointerToExtract];

  incomingPacket.packetPusher->pushPacket(
      move(packetWithExtractedParameter),
      kChannel_ExtractedParameter);
}

}  // namespace maplang
