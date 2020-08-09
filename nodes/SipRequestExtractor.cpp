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

#include "SipRequestExtractor.h"

using namespace std;
using namespace nlohmann;

namespace maplang {

static const string kParameter_SipMethod = "sipMethod";
static const string kParameter_SipRequestUri = "sipRequestUri";
static const string kParameter_SipVersion = "sipVersion";
static const string kParameter_SipHeaders = "sipHeaders";

static string toLower(const string& str) {
  ostringstream out;
  size_t length = str.length();
  for (size_t i = 0; i < length; i++) {
    out.put(tolower(str[i]));
  }

  return out.str();
}

void SipRequestExtractor::handlePacket(const Packet* incomingPacket) {
  mMessageStream.append(incomingPacket->buffers[0]);

  bool keepGoing;
  do {
    if (!mProcessedHead) {
      keepGoing = handleHeaderData();
    } else {
      keepGoing = handleBodyData();
    }
  } while (keepGoing);
}

bool SipRequestExtractor::handleHeaderData() {
  static constexpr const char *const kCrLf = "\r\n";
  static constexpr size_t kCrLfStrLen = sizeof(kCrLf) - 1;
  static constexpr const char *const kDoubleCrLf = "\r\n\r\n";
  static constexpr size_t kDoubleCrLfStrLen = sizeof(kDoubleCrLf) - 1;

  const size_t endOfHeaders = mMessageStream.firstIndexOf(kDoubleCrLf, kDoubleCrLfStrLen);
  if (endOfHeaders == MemoryStream::kNotFound) {
    const bool keepGoing = false;
    return keepGoing;
  }

  vector<MemoryStream> headAndBody = mMessageStream.splitIntoMemoryStreams(kDoubleCrLf, kDoubleCrLfStrLen, 2);
  const MemoryStream& head = headAndBody[0];
  const MemoryStream& body = headAndBody[1];

  vector<MemoryStream> lines = head.splitIntoMemoryStreams(kCrLf, kCrLfStrLen);
  if (!lines.empty()) {
    const auto& line = lines[0];

    vector<string> lineElements = line.splitIntoStrings(' ', 3);
    mMethod = lineElements[0];
    mPendingPacket.parameters[kParameter_SipMethod] = mMethod;
    mPendingPacket.parameters[kParameter_SipRequestUri] = lineElements[1];
    mPendingPacket.parameters[kParameter_SipVersion] = lineElements[2];
  }

  for (size_t i = 0; i < lines.size(); i++) {
    const auto& line = lines[i];
    vector<MemoryStream> keyValue = line.splitIntoMemoryStreams(':', 2);
    const string& key = toLower(keyValue[0].trim().asString());
    string value;
    if (keyValue.size() > 1) {
      value = keyValue[1].trim().asString();
    }

    mPendingPacket.parameters[kParameter_SipHeaders][key] = value;
    if (key == "content-length") {
      mContentLength = stoull(value);
    }
  }

  mMessageStream = mMessageStream.subStream(endOfHeaders + kDoubleCrLfStrLen);

  const bool haveCompleteMessage = mContentLength == 0;
  if (haveCompleteMessage) {
    mPacketPusher->pushPacket(&mPendingPacket, mMethod);
    reset();
  } else {
    mProcessedHead = true;
  }

  const bool keepGoing = haveCompleteMessage && mMessageStream.size() > 0;
  return keepGoing;
}

bool SipRequestExtractor::handleBodyData() {
  if (mMessageStream.size() < mContentLength) {
    const bool keepGoing = false;
    return keepGoing;
  }

  Buffer body;
  body.length = mContentLength;
  body.data = shared_ptr<uint8_t>(new uint8_t[mContentLength], default_delete<uint8_t[]>());
  mMessageStream.read(0, mContentLength, &body);

  mPendingPacket.buffers.emplace_back(move(body));
  mPacketPusher->pushPacket(&mPendingPacket, mMethod);

  mMessageStream = mMessageStream.subStream(mContentLength);
  reset();

  const bool keepGoing = mMessageStream.size() > 0;
  return keepGoing;
}

void SipRequestExtractor::setPacketPusher(const std::shared_ptr<IPacketPusher>& pusher) {
  mPacketPusher = pusher;
}

void SipRequestExtractor::reset() {
  mPendingPacket = Packet();
  mMethod = "";
  mProcessedHead = false;
  mContentLength = 0;
}

}  // namespace maplang
