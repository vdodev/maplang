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

#include "nodes/HttpResponseWithAddressAsBody.h"

using namespace std;
using namespace nlohmann;

namespace maplang {

static const char* const kParameter_HttpHeaders = "http-headers";
static const char* const kParameter_HttpStatusCode = "http-status-code";
static const char* const kParameter_HttpStatusMessage = "http-status-message";

static const char* const kParameter_RemoteAddress = "RemoteAddress";


void HttpResponseWithAddressAsBody::handlePacket(const PathablePacket* incomingPacket) {
  string address = "unknown";

  if (incomingPacket->parameters.find(kParameter_RemoteAddress) != incomingPacket->parameters.end()) {
    address = incomingPacket->parameters[kParameter_RemoteAddress];
  }

  shared_ptr<uint8_t> body = shared_ptr<uint8_t>(new uint8_t[address.length()],
                                                 default_delete<uint8_t[]>());

  address.copy(reinterpret_cast<char*>(body.get()), address.length());
  Packet response;
  response.parameters = incomingPacket->parameters;
  json httpHeaders;
  httpHeaders["Content-Type"] = "text/plain";
  response.parameters[kParameter_HttpHeaders] = httpHeaders;
  response.parameters[kParameter_HttpStatusCode] = 200;
  response.parameters[kParameter_HttpStatusMessage] = "OK";

  response.buffers.push_back(Buffer(body, address.length()));

  incomingPacket->packetPusher->pushPacket(&response, "On Response");
}

/*
 * Back-propagation of required parameters.
 *
 * Cause parameters to be retained that can be used by down-stream nodes.
 * For each sink, apply the back-propagation broadcast algorithm.
 * For each node
 */

}  // namespace maplang
