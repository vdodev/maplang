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

#ifndef DATA_GRAPH_NODES_PACKETREADER_H_
#define DATA_GRAPH_NODES_PACKETREADER_H_

#include "../IPathable.h"
#include "../INode.h"
#include "../MemoryStream.h"

namespace dgraph {

class PacketReader : public INode, public IPathable {
 public:

  void handlePacket(const PathablePacket *incomingPacket) override;

  IPathable *asPathable() override { return this; }
  ISink *asSink() override { return nullptr; }
  ISource *asSource() override { return nullptr; }

 private:
  uint64_t mLength = 0;
  MemoryStream mPendingBytes;

  static Packet readPacket(const MemoryStream& stream);
};

}  // namespace dgraph

#endif //DATA_GRAPH_NODES_PACKETREADER_H_
