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

#pragma once

#include <list>
#include <random>

#include "../ContextualNode.h"
#include "../ISource.h"
#include "../MemoryStream.h"
#include "../json.hpp"

namespace dgraph {

class HttpRequestExtractor final : public INode, public ISink, public ISource {
 public:
  HttpRequestExtractor(const nlohmann::json& parameters);
  ~HttpRequestExtractor() override;

  void handlePacket(const Packet* packet) override;
  void setPacketPusher(const std::shared_ptr<IPacketPusher>& pusher) override;

  IPathable *asPathable() override { return nullptr; }
  ISink *asSink() override { return this; }
  ISource *asSource() override { return this; }

 private:
  std::shared_ptr<IPacketPusher> mPacketPusher;
  std::random_device mRandomDevice;
  std::uniform_int_distribution<uint64_t> mUniformDistribution;
  bool mSentHeaders;
  MemoryStream mHeaderData;
  std::string mRequestId;
  size_t mBodyLength;
  size_t mSentBodyDataByteCount;

  void reset();
  static nlohmann::json parseHeaders(const MemoryStream& headers);
  Packet createHeaderPacket(const MemoryStream& memoryStream) const;
  Packet createBodyPacket(const Buffer& bodyBuffer) const;

  void sendEndOfRequestPacketIfRequestPending();
};

}  // namespace dgraph
