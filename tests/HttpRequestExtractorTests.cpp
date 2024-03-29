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

#include <functional>

#include "gtest/gtest.h"
#include "maplang/FactoriesBuilder.h"
#include "maplang/HttpUtilities.h"
#include "maplang/LambdaPacketPusher.h"
#include "nodes/HttpRequestExtractor.h"

using namespace std;
using namespace nlohmann;

namespace maplang {

class HttpRequestExtractorTests : public testing::Test {
 public:
  HttpRequestExtractorTests()
      : mFactories(FactoriesBuilder().BuildFactories()) {}

  const Factories mFactories;
};

TEST_F(
    HttpRequestExtractorTests,
    WhenAnHttpRequestIsProcessed_HeaderFieldsAndBodyAreCorrect) {
  auto extractor = make_shared<HttpRequestExtractor>(mFactories, json());

  Buffer buffer("GET / HTTP/1.1\r\nheaderKey: headerValue\r\n\r\nHi");

  Packet packet;
  packet.buffers.emplace_back(buffer);

  Buffer bodyOnlyBuffer(", hello.");
  Packet bodyOnlyPacket;
  bodyOnlyPacket.buffers.emplace_back(bodyOnlyBuffer);

  size_t receivedHeaderPacketCount = 0;
  Packet headerPacket;
  vector<Packet> bodyPackets;
  string lastUnexpectedChannel;
  size_t unexpectedChannelCount = 0;
  size_t requestEndedPacketCount = 0;
  Packet requestEndedPacket;
  const auto packetPusher = make_shared<LambdaPacketPusher>(
      [&receivedHeaderPacketCount,
       &headerPacket,
       &bodyPackets,
       &lastUnexpectedChannel,
       &requestEndedPacketCount,
       &requestEndedPacket,
       &unexpectedChannelCount](const Packet& packet, const string& channel) {
        if (channel == "New Request") {
          headerPacket = packet;
          receivedHeaderPacketCount++;
        } else if (channel == "Body Data") {
          bodyPackets.push_back(packet);
        } else if (channel == "Request Ended") {
          requestEndedPacketCount++;
          requestEndedPacket = packet;
        } else {
          lastUnexpectedChannel = channel;
          unexpectedChannelCount++;
        }
      });

  extractor->handlePacket(PathablePacket(packet, packetPusher));
  extractor->handlePacket(PathablePacket(bodyOnlyPacket, packetPusher));

  ASSERT_EQ(1, receivedHeaderPacketCount);
  ASSERT_EQ(0, unexpectedChannelCount)
      << "Unexpected channel '" << lastUnexpectedChannel << "'.";

  ASSERT_TRUE(headerPacket.parameters.contains(http::kParameter_HttpRequestId));
  const string requestId =
      headerPacket.parameters[http::kParameter_HttpRequestId];

  ASSERT_TRUE(headerPacket.parameters.contains(http::kParameter_HttpMethod));
  const string method =
      headerPacket.parameters[http::kParameter_HttpMethod].get<string>();
  ASSERT_STREQ("GET", method.c_str());

  ASSERT_TRUE(headerPacket.parameters.contains(http::kParameter_HttpPath));
  const string path =
      headerPacket.parameters[http::kParameter_HttpPath].get<string>();
  ASSERT_STREQ("/", path.c_str());

  ASSERT_TRUE(headerPacket.parameters.contains(http::kParameter_HttpVersion));
  const string httpVersion =
      headerPacket.parameters[http::kParameter_HttpVersion].get<string>();
  ASSERT_STREQ("HTTP/1.1", httpVersion.c_str());

  ASSERT_TRUE(headerPacket.parameters.contains(http::kParameter_HttpHeaders));
  const json& headers = headerPacket.parameters[http::kParameter_HttpHeaders];
  ASSERT_TRUE(headers.is_object());

  bool foundHeaderKey = false;
  for (auto it = headers.begin(); it != headers.end(); it++) {
    string headerKey = it.key();
    if (headerKey == "headerkey") {
      ASSERT_FALSE(foundHeaderKey);
      string headerValue = headers["headerkey"].get<string>();
      ASSERT_STREQ("headerValue", headerValue.c_str());

      foundHeaderKey = true;
    } else {
      FAIL() << "Unexpected header '" << headerKey << "'.";
    }
  }

  ASSERT_EQ(2, bodyPackets.size());

  for (size_t i = 0; i < bodyPackets.size(); i++) {
    const auto& bodyPacket = bodyPackets[i];
    ASSERT_EQ(1, bodyPacket.buffers.size()) << "Packet " << i;

    ASSERT_TRUE(bodyPacket.parameters.contains(http::kParameter_HttpRequestId))
        << "Packet " << i;
    const string thisPacketsRequestId =
        bodyPacket.parameters[http::kParameter_HttpRequestId];
    ASSERT_STREQ(requestId.c_str(), thisPacketsRequestId.c_str())
        << "Packet " << i;
  }

  ASSERT_TRUE(
      0
      == strncmp(
          "Hi",
          reinterpret_cast<const char*>(bodyPackets[0].buffers[0].data.get()),
          strlen("Hi")));
  ASSERT_TRUE(
      0
      == strncmp(
          ", hello.",
          reinterpret_cast<const char*>(bodyPackets[1].buffers[0].data.get()),
          strlen(", hello.")));

  extractor.reset();
  ASSERT_EQ(1, requestEndedPacketCount);

  ASSERT_TRUE(
      requestEndedPacket.parameters.contains(http::kParameter_HttpRequestId));
  const string requestEndedPacketsRequestId =
      requestEndedPacket.parameters[http::kParameter_HttpRequestId];
  ASSERT_STREQ(requestId.c_str(), requestEndedPacketsRequestId.c_str());
}

}  // namespace maplang
