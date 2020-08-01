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

#ifndef DATA_GRAPH_NODES_SIPREQUESTEXTRACTOR_H_
#define DATA_GRAPH_NODES_SIPREQUESTEXTRACTOR_H_

#include "../INode.h"
#include "../ISink.h"
#include "../ISource.h"
#include "../MemoryStream.h"

namespace dgraph {

class SipRequestExtractor final : public INode, public ISink, public ISource {
 public:
  void handlePacket(const Packet* incomingPacket) override;

  IPathable *asPathable() override { return nullptr; }
  ISink *asSink() override { return this; }
  ISource *asSource() override { return this; }

  void setPacketPusher(const std::shared_ptr<IPacketPusher>& pusher) override;

 private:
  std::shared_ptr<IPacketPusher> mPacketPusher;
  MemoryStream mMessageStream;
  Packet mPendingPacket;
  std::string mMethod;
  bool mProcessedHead = false;
  size_t mContentLength = 0;

  bool handleHeaderData();
  bool handleBodyData();

  void reset();
};

}  // namespace dgraph

#endif //DATA_GRAPH_NODES_SIPREQUESTEXTRACTOR_H_
