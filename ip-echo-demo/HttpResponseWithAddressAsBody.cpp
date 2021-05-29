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

#include "HttpResponseWithAddressAsBody.h"

#include "maplang/HttpUtilities.h"

using namespace std;
using namespace nlohmann;

namespace maplang {

static const char* const kParameter_RemoteAddress = "RemoteAddress";

void HttpResponseWithAddressAsBody::handlePacket(
    const PathablePacket& incomingPathablePacket) {
  const Packet& incomingPacket = incomingPathablePacket.packet;
  string address = "unknown";

  if (incomingPacket.parameters.find(kParameter_RemoteAddress)
      != incomingPacket.parameters.end()) {
    address = incomingPacket.parameters[kParameter_RemoteAddress];
  }

  auto body = shared_ptr<uint8_t[]>(new uint8_t[address.length()]);

  address.copy(reinterpret_cast<char*>(body.get()), address.length());
  Packet response;
  response.parameters = incomingPacket.parameters;
  json httpHeaders;
  httpHeaders[http::kHttpHeaderNormalized_ContentType] = "text/plain";
  response.parameters[http::kParameter_HttpHeaders] = httpHeaders;
  response.parameters[http::kParameter_HttpStatusCode] = http::kHttpStatus_Ok;

  response.buffers.push_back(Buffer(body, address.length()));

  incomingPathablePacket.packetPusher->pushPacket(
      move(response),
      "On Response");
}

}  // namespace maplang
