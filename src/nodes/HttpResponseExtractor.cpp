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

#include <memory>

#include "nodes/HttpResponseExtractor.h"
#include "maplang/Errors.h"
#include "maplang/HttpUtilities.h"

using namespace std;
using namespace nlohmann;

namespace maplang {

static const string kChannel_BodyData = "Body Data";
static const string kChannel_ResponseEnded = "Request Ended";
static const string kChannel_ReponseHeadersReceived = "Reponse Headers Received";

HttpResponseExtractor::HttpResponseExtractor(const nlohmann::json& parameters) {
  reset();
}

HttpResponseExtractor::~HttpResponseExtractor() {
  sendEndOfRequestPacketIfRequestPending();
}

void HttpResponseExtractor::setPacketPusher(
    const std::shared_ptr<IPacketPusher>& pusher) {
  mPacketPusher = pusher;
}

void HttpResponseExtractor::handlePacket(const Packet& incomingPacket) {
  try {
    if (mReceivedHeaders) {
      bool knownLastBufferInRequest = false;
      const Buffer& incomingBuffer = incomingPacket.buffers[0];
      Packet bodyPacket =
          createBodyPacket(incomingPacket.buffers[0]);
      if (mBodyLength != SIZE_MAX) {
        const size_t remainingBodyLength = mBodyLength - mSentBodyDataByteCount;
        knownLastBufferInRequest = incomingBuffer.length <= remainingBodyLength;

        if (knownLastBufferInRequest) {
          bodyPacket.buffers[0].length = remainingBodyLength;
        }
      }

      mPacketPusher->pushPacket(move(bodyPacket), kChannel_BodyData);

      if (knownLastBufferInRequest) {
        sendEndOfRequestPacketIfRequestPending();
        reset();
      }

      return;
    }

    const size_t bufferSizeBeforeAppending = mHeaderData.size();
    mHeaderData.append(incomingPacket.buffers[0]);

    static constexpr char kDoubleCrLf[] = "\r\n\r\n";
    static constexpr size_t kDoubleCrLfLength = sizeof(kDoubleCrLf) - 1;
    const size_t headersEnd =
        mHeaderData.firstIndexOf(kDoubleCrLf, kDoubleCrLfLength);
    if (headersEnd == MemoryStream::kNotFound) {
      return;
    }

    // Send a packet containing the request headers (no body data).
    Packet headerPacket = createHeaderPacket(mHeaderData.subStream(0, headersEnd));
    size_t contentLength = SIZE_MAX;
    if (headerPacket.parameters[http::kParameter_HttpHeaders].contains(
        "content-length")) {
      contentLength =
          headerPacket.parameters[http::kParameter_HttpHeaders][http::kHttpHeaderNormalized_ContentLength]
              .get<size_t>();
    }

    mPacketPusher->pushPacket(move(headerPacket), kChannel_ReponseHeadersReceived);
    mReceivedHeaders = true;

    // If the incoming packet has part of the body, send body data as a separate
    // packet.
    const size_t bodyStart = headersEnd + kDoubleCrLfLength;
    const size_t availableBodyLength = mHeaderData.size() - bodyStart;
    if (availableBodyLength > 0) {
      const size_t offsetOfBodyInLastBuffer =
          bodyStart - bufferSizeBeforeAppending;
      uint8_t* body =
          incomingPacket.buffers[0].data.get() + offsetOfBodyInLastBuffer;

      Buffer bodyBuffer;
      bodyBuffer.data =
          shared_ptr<uint8_t>(incomingPacket.buffers[0].data, body);

      const size_t bodyLength = availableBodyLength < contentLength
                                ? availableBodyLength
                                : contentLength;
      bodyBuffer.length = bodyLength;

      Packet bodyPacket = createBodyPacket(bodyBuffer);
      mPacketPusher->pushPacket(move(bodyPacket), kChannel_BodyData);
      mSentBodyDataByteCount += bodyBuffer.length;
    }

    mHeaderData.clear();
  } catch (const exception& ex) {
    sendErrorPacket(mPacketPusher, ex);
  }
}

Packet HttpResponseExtractor::createHeaderPacket(const MemoryStream& memoryStream) const {
  MemoryStream firstLine;
  MemoryStream headersStream;

  size_t firstNonCrLfIndex = memoryStream.firstIndexNotOfAnyInSet("\r\n", 2);
  MemoryStream trimmedStream = memoryStream.subStream(firstNonCrLfIndex);
  memoryStream.split(
      "\r\n", 2,
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
      parameters[http::kParameter_HttpVersion] = tokenString;
    } else if (index == 1) {
      parameters[http::kParameter_HttpStatusCode] = tokenString;
    } else if (index == 2) {
      parameters[http::kParameter_HttpStatusReason] = tokenString;
    }

    return true;
  });

  parameters[http::kParameter_HttpRequestId] = mRequestId;

  Packet headerPacket;
  headerPacket.parameters = parameters;

  return headerPacket;
}

void HttpResponseExtractor::reset() {
  mReceivedHeaders = false;
  mHeaderData.clear();
  mRequestId = to_string(mUniformDistribution(mRandomDevice));
  mBodyLength = SIZE_MAX;
  mSentBodyDataByteCount = 0;
}

Packet HttpResponseExtractor::createBodyPacket(const Buffer& bodyBuffer) const {
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

json HttpResponseExtractor::parseHeaders(const MemoryStream& headers) {
  json parsedHeaders;
  headers.split(
      "\r\n", 2, [&parsedHeaders](size_t lineIndex, MemoryStream&& headerLine) {
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

void HttpResponseExtractor::sendEndOfRequestPacketIfRequestPending() {
  if (!mReceivedHeaders) {
    return;
  }

  Packet packet;
  packet.parameters[http::kParameter_HttpRequestId] = mRequestId;

  if (mPacketPusher) {
    mPacketPusher->pushPacket(move(packet), kChannel_ResponseEnded);
  }
}

}  // namespace maplang
