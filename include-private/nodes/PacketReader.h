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

#ifndef MAPLANG_PACKETREADER_H_
#define MAPLANG_PACKETREADER_H_

#include "maplang/IImplementation.h"
#include "maplang/IPathable.h"
#include "maplang/MemoryStream.h"
#include "maplang/Factories.h"

namespace maplang {

class PacketReader : public IImplementation, public IPathable {
 public:
  PacketReader(const Factories& factories);
  ~PacketReader() override = default;

  void handlePacket(const PathablePacket& incomingPacket) override;

  IPathable* asPathable() override { return this; }
  ISource* asSource() override { return nullptr; }
  IGroup* asGroup() override { return nullptr; }

 private:
  const Factories mFactories;

  uint64_t mLength = 0;
  MemoryStream mPendingBytes;

  Packet readPacket(const MemoryStream& stream) const;
};

}  // namespace maplang

#endif  // MAPLANG_PACKETREADER_H_
