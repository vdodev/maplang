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

#include "nodes/HttpRequestExtractor.h"

#include <memory>

#include "maplang/Errors.h"
#include "maplang/HttpUtilities.h"

using namespace std;
using namespace nlohmann;

namespace maplang {

static const string kChannel_BodyData = "Body Data";
static const string kChannel_RequestEnded = "Request Ended";
static const string kChannel_NewRequest = "New Request";

HttpRequestExtractor::HttpRequestExtractor(
    const Factories& factories,
    const nlohmann::json& initParameters)
    : mFactories(factories), mInitParameters(initParameters) {
  reset();
}

HttpRequestExtractor::~HttpRequestExtractor() noexcept {
  this->sendEndOfRequestPacketIfRequestPending(mLastPayloadsPacketPusher);
}

void HttpRequestExtractor::handlePacket(const PathablePacket& incomingPacket) {
  try {
    mLastPayloadsPacketPusher = incomingPacket.packetPusher;

    if (mSentHeaders) {
      /*
       * Won't know if it's the last one when Content-Length isn't set, but that
       * means a new connection is required for the next request. Because this
       * node is usually used inside of a ContextualNode, another instance of
       * this class will handle the next request.
       */
      bool knownLastBufferInRequest = false;
      const Buffer& incomingBuffer = incomingPacket.packet.buffers[0];
      Packet bodyPacket = createBodyPacket(incomingPacket.packet.buffers[0]);
      if (mBodyLength != SIZE_MAX) {
        const size_t remainingBodyLength = mBodyLength - mSentBodyDataByteCount;
        knownLastBufferInRequest = incomingBuffer.length <= remainingBodyLength;

        if (knownLastBufferInRequest) {
          bodyPacket.buffers[0].length = remainingBodyLength;
        }
      }

      incomingPacket.packetPusher->pushPacket(
          move(bodyPacket),
          kChannel_BodyData);

      if (knownLastBufferInRequest) {
        sendEndOfRequestPacketIfRequestPending(incomingPacket.packetPusher);
        reset();
      }

      return;
    }

    const size_t bufferSizeBeforeAppending = mHeaderData.size();
    mHeaderData.append(incomingPacket.packet.buffers[0]);

    static constexpr char kDoubleCrLf[] = "\r\n\r\n";
    static constexpr size_t kDoubleCrLfLength = sizeof(kDoubleCrLf) - 1;
    const size_t headersEnd =
        mHeaderData.firstIndexOf(kDoubleCrLf, kDoubleCrLfLength);
    if (headersEnd == MemoryStream::kNotFound) {
      return;
    }

    // Send a packet containing the request headers (no body data).
    Packet headerPacket =
        createHeaderPacket(mHeaderData.subStream(0, headersEnd));
    size_t contentLength = SIZE_MAX;
    const json& httpHeaders =
        headerPacket.parameters[http::kParameter_HttpHeaders];
    if (httpHeaders.contains(http::kHttpHeaderNormalized_ContentLength)) {
      contentLength = headerPacket
                          .parameters[http::kParameter_HttpHeaders]
                                     [http::kHttpHeaderNormalized_ContentLength]
                          .get<uint64_t>();
    }

    incomingPacket.packetPusher->pushPacket(
        move(headerPacket),
        kChannel_NewRequest);
    mSentHeaders = true;

    // If the incoming packet has part of the body, send body data as a separate
    // packet.
    const size_t bodyStart = headersEnd + kDoubleCrLfLength;
    const size_t availableBodyLength = mHeaderData.size() - bodyStart;
    if (availableBodyLength > 0) {
      const size_t offsetOfBodyInLastBuffer =
          bodyStart - bufferSizeBeforeAppending;

      Buffer bodyBuffer = incomingPacket.packet.buffers[0].slice(offsetOfBodyInLastBuffer);

      incomingPacket.packetPusher->pushPacket(
          createBodyPacket(bodyBuffer),
          kChannel_BodyData);

      mSentBodyDataByteCount += bodyBuffer.length;
    }

    mHeaderData.clear();
  } catch (const exception& ex) {
    sendErrorPacket(incomingPacket.packetPusher, ex);
  }
}

Packet HttpRequestExtractor::createHeaderPacket(
    const MemoryStream& memoryStream) const {
  MemoryStream firstLine;
  MemoryStream headersStream;

  size_t firstNonCrLfIndex = memoryStream.firstIndexNotOfAnyInSet("\r\n", 2);
  MemoryStream trimmedStream = memoryStream.subStream(firstNonCrLfIndex);
  memoryStream.split(
      "\r\n",
      2,
      [&firstLine, &headersStream](size_t index, MemoryStream&& stream) {
        if (index == 0) {
          firstLine = move(stream);
        } else {
          headersStream = move(stream);
        }

        return true;
      });

  json parameters;

  parameters[http::kParameter_HttpHeaders] = parseHeaders(headersStream);
  firstLine.split(' ', [&parameters](size_t index, MemoryStream&& token) {
    ostringstream outStream;
    outStream << token;
    string tokenString = outStream.str();

    if (index == 0) {
      parameters[http::kParameter_HttpMethod] = tokenString;
    } else if (index == 1) {
      parameters[http::kParameter_HttpPath] = tokenString;
    } else if (index == 2) {
      parameters[http::kParameter_HttpVersion] = tokenString;
    }

    return true;
  });

  parameters[http::kParameter_HttpRequestId] = mRequestId;

  Packet headerPacket;
  headerPacket.parameters = parameters;

  return headerPacket;
}

void HttpRequestExtractor::reset() {
  mSentHeaders = false;
  mHeaderData.clear();
  mRequestId = to_string(mUniformDistribution(mRandomDevice));
  mBodyLength = SIZE_MAX;
  mSentBodyDataByteCount = 0;
}

Packet HttpRequestExtractor::createBodyPacket(const Buffer& bodyBuffer) const {
  Packet bodyPacket;
  bodyPacket.buffers.push_back(bodyBuffer);
  bodyPacket.parameters[http::kParameter_HttpRequestId] = mRequestId;

  return bodyPacket;
}

static string toLower(const string& str) {
  ostringstream out;
  size_t length = str.length();
  for (size_t i = 0; i < length; i++) {
    out.put(tolower(str[i]));
  }

  return out.str();
}

json HttpRequestExtractor::parseHeaders(const MemoryStream& headers) {
  json parsedHeaders;
  headers.split(
      "\r\n",
      2,
      [&parsedHeaders](size_t lineIndex, MemoryStream&& headerLine) {
        static constexpr size_t maxTokens = 2;
        ostringstream key;
        ostringstream value;
        headerLine.split(
            ':',
            [&key, &value](size_t kvIndex, MemoryStream&& keyOrValue) {
              if (kvIndex == 0) {
                key << keyOrValue.trim();
              } else {
                value << keyOrValue.trim();
              }

              return true;
            },
            maxTokens);

        parsedHeaders[toLower(key.str())] = value.str();
        return true;  // keep iterating until the end
      });

  return parsedHeaders;
}

void HttpRequestExtractor::sendEndOfRequestPacketIfRequestPending(
    const shared_ptr<IPacketPusher>& packetPusher) {
  if (!mSentHeaders) {
    return;
  }

  Packet packet;
  packet.parameters[http::kParameter_HttpRequestId] = mRequestId;

  packetPusher->pushPacket(move(packet), kChannel_RequestEnded);
}

}  // namespace maplang
