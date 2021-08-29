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

#include "nodes/HttpResponseWriter.h"

#include <sstream>

#include "logging.h"
#include "maplang/HttpUtilities.h"

using namespace std;
using namespace nlohmann;

namespace maplang {

static const char* const kChannel_HttpData = "Http Data";

HttpResponseWriter::HttpResponseWriter(
    const Factories& factories,
    const nlohmann::json& initParameters)
    : mFactories(factories), mInitParameters(initParameters) {}

void HttpResponseWriter::handlePacket(const PathablePacket& pathablePacket) {
  const auto& packet = pathablePacket.packet;
  const auto& packetPusher = pathablePacket.packetPusher;

  stringstream httpBytes;
  auto statusCode =
      packet.parameters[http::kParameter_HttpStatusCode].get<uint64_t>();
  string statusReason;
  if (packet.parameters.contains(http::kParameter_HttpStatusReason)) {
    statusReason =
        packet.parameters[http::kParameter_HttpStatusReason].get<string>();
  } else {
    statusReason = http::getDefaultReasonForHttpStatus(statusCode);
  }

  static const char* const CRLF = "\r\n";
  httpBytes << "HTTP/1.1 " << statusCode << " " << statusReason << CRLF;

  json httpHeaders = packet.parameters[http::kParameter_HttpHeaders];

  if (!packet.buffers.empty() && packet.buffers[0].length > 0) {
    const size_t contentLength = packet.buffers[0].length;
    httpHeaders[http::kHttpHeaderNormalized_ContentLength] =
        to_string(contentLength);
  } else if (httpHeaders.contains(http::kHttpHeaderNormalized_ContentLength)) {
    httpHeaders.erase(http::kHttpHeaderNormalized_ContentLength);
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

  if (!packet.buffers.empty()) {
    const Buffer& buffer = packet.buffers[0];
    httpBytes.write((const char*)buffer.data.get(), buffer.length);

    httpBytes.seekg(0, ios::end);
    const size_t bufferLength = httpBytes.tellg();
    httpBytes.seekg(0, ios::beg);

    auto bodyData = shared_ptr<uint8_t[]>(new uint8_t[bufferLength]);
    httpBytes.read(reinterpret_cast<char*>(bodyData.get()), bufferLength);
    Buffer bodyBuffer(bodyData, bufferLength);
    httpBytesPacket.buffers.push_back(bodyBuffer);
  }

  packetPusher->pushPacket(move(httpBytesPacket), kChannel_HttpData);
}

}  // namespace maplang
