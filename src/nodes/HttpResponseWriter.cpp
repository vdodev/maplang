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

#include "HttpResponseWriter.h"
#include "../logging.h"
#include <sstream>

using namespace std;
using namespace nlohmann;

namespace maplang {

static const char* const kChannel_HttpData = "Http Data";
static const char* const kParameter_HttpHeaders = "http-headers";

void HttpResponseWriter::setPacketPusher(
    const std::shared_ptr<IPacketPusher>& packetPusher) {
  mPacketPusher = packetPusher;
}

void HttpResponseWriter::handlePacket(const Packet* packet) {
  stringstream httpBytes;
  auto statusCode = packet->parameters["http-status-code"].get<uint32_t>();
  auto statusMessage = packet->parameters["http-status-message"].get<string>();

  static const char* const CRLF = "\r\n";
  httpBytes << "HTTP/1.1 " << statusCode << " " << statusMessage << CRLF;

  json httpHeaders = packet->parameters[kParameter_HttpHeaders];

  if (!packet->buffers.empty() && packet->buffers[0].length > 0) {
    const size_t contentLength = packet->buffers[0].length;
    httpHeaders["Content-Length"] = to_string(contentLength);
  }

  const auto httpHeadersEnd = httpHeaders.end();
  for (auto it = httpHeaders.begin(); it != httpHeadersEnd; it++) {
    if (it.key().empty()) {
      continue;
    }

    httpBytes << it.key() << ": " << it.value().get<string>() << CRLF;
  }
  httpBytes << CRLF;

  Packet httpBytesPacket;

  if (!packet->buffers.empty()) {
    const Buffer& buffer = packet->buffers[0];
    httpBytes.write((const char*)buffer.data.get(), buffer.length);

    httpBytes.seekg(0, ios::end);
    const size_t bufferLength = httpBytes.tellg();
    httpBytes.seekg(0, ios::beg);

    auto bodyData = shared_ptr<uint8_t>(new uint8_t[bufferLength],
                                        default_delete<uint8_t[]>());
    httpBytes.read(reinterpret_cast<char*>(bodyData.get()), bufferLength);
    Buffer bodyBuffer(bodyData, bufferLength);
    httpBytesPacket.buffers.push_back(bodyBuffer);
  }

  mPacketPusher->pushPacket(&httpBytesPacket, kChannel_HttpData);
}

}  // namespace maplang
