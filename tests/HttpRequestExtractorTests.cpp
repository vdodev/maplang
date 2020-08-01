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
#include "nodes/HttpRequestExtractor.h"

using namespace std;
using namespace nlohmann;

namespace dgraph {

class LambdaPacketPusher : public IPacketPusher {
 public:
  LambdaPacketPusher(
      function<void(const Packet* packet, const string& channel)>&& onPacket)
      : mOnPacket(move(onPacket)) {}

  ~LambdaPacketPusher() override = default;

  void pushPacket(const Packet* packet, const string& channel) override {
    mOnPacket(packet, channel);
  }

 private:
  function<void(const Packet* packet, const string& channel)> mOnPacket;
};

TEST(WhenAnHttpRequestIsProcessed, HeaderFieldsAndBodyAreCorrect) {
  auto extractor = make_shared<HttpRequestExtractor>(nullptr);

  Buffer buffer;
  char bufferContents[] = "GET / HTTP/1.1\r\nheaderKey: headerValue\r\n\r\nHi";

  buffer.data = shared_ptr<uint8_t>(reinterpret_cast<uint8_t*>(bufferContents),
                                    [](uint8_t*) {});
  buffer.length = strlen(bufferContents);

  Packet packet;
  packet.buffers.emplace_back(buffer);

  Buffer bodyOnlyBuffer;
  char bodyOnlyBufferContents[] = ", hello.";
  bodyOnlyBuffer.data = shared_ptr<uint8_t>(
      reinterpret_cast<uint8_t*>(bodyOnlyBufferContents), [](uint8_t*) {});
  bodyOnlyBuffer.length = strlen(bufferContents);
  Packet bodyOnlyPacket;
  bodyOnlyPacket.buffers.emplace_back(bodyOnlyBuffer);

  size_t receivedHeaderPacketCount = 0;
  Packet headerPacket;
  vector<Packet> bodyPackets;
  string lastUnexpectedChannel;
  size_t unexpectedChannelCount = 0;
  size_t requestEndedPacketCount = 0;
  Packet requestEndedPacket;
  extractor->setPacketPusher(make_shared<LambdaPacketPusher>(
      [&receivedHeaderPacketCount, &headerPacket, &bodyPackets,
       &lastUnexpectedChannel, &requestEndedPacketCount, &requestEndedPacket,
       &unexpectedChannelCount](const Packet* packet, const string& channel) {
        if (channel == "New Request") {
          headerPacket = *packet;
          receivedHeaderPacketCount++;
        } else if (channel == "Body Data") {
          bodyPackets.push_back(*packet);
        } else if (channel == "Request Ended") {
          requestEndedPacketCount++;
          requestEndedPacket = *packet;
        } else {
          lastUnexpectedChannel = channel;
          unexpectedChannelCount++;
        }
      }));

  extractor->handlePacket(&packet);
  extractor->handlePacket(&bodyOnlyPacket);

  ASSERT_EQ(1, receivedHeaderPacketCount);
  ASSERT_EQ(0, unexpectedChannelCount)
      << "Unexpected channel '" << lastUnexpectedChannel << "'.";

  ASSERT_NE(headerPacket.parameters.end(),
            headerPacket.parameters.find("httpRequestId"));
  const string requestId = headerPacket.parameters["httpRequestId"];

  ASSERT_NE(headerPacket.parameters.end(),
            headerPacket.parameters.find("method"));
  const string method = headerPacket.parameters["method"].get<string>();
  ASSERT_STREQ("GET", method.c_str());

  ASSERT_NE(headerPacket.parameters.end(),
            headerPacket.parameters.find("path"));
  const string path = headerPacket.parameters["path"].get<string>();
  ASSERT_STREQ("/", path.c_str());

  ASSERT_NE(headerPacket.parameters.end(),
            headerPacket.parameters.find("http-version"));
  const string httpVersion =
      headerPacket.parameters["http-version"].get<string>();
  ASSERT_STREQ("HTTP/1.1", httpVersion.c_str());

  ASSERT_NE(headerPacket.parameters.end(),
            headerPacket.parameters.find("http-headers"));
  const json& headers = headerPacket.parameters["http-headers"];
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

    ASSERT_NE(bodyPacket.parameters.end(),
              bodyPacket.parameters.find("httpRequestId"))
        << "Packet " << i;
    const string thisPacketsRequestId = bodyPacket.parameters["httpRequestId"];
    ASSERT_STREQ(requestId.c_str(), thisPacketsRequestId.c_str())
        << "Packet " << i;
  }

  ASSERT_STREQ("Hi", reinterpret_cast<const char*>(
                         bodyPackets[0].buffers[0].data.get()));
  ASSERT_STREQ(", hello.", reinterpret_cast<const char*>(
                               bodyPackets[1].buffers[0].data.get()));

  extractor.reset();
  ASSERT_EQ(1, requestEndedPacketCount);

  ASSERT_NE(requestEndedPacket.parameters.end(),
            requestEndedPacket.parameters.find("httpRequestId"));
  const string requestEndedPacketsRequestId =
      requestEndedPacket.parameters["httpRequestId"];
  ASSERT_STREQ(requestId.c_str(), requestEndedPacketsRequestId.c_str());
}

}  // namespace dgraph
