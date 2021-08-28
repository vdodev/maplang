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

#ifndef MAPLANG_INCLUDE_MAPLANG_SIMPLESOURCE_H_
#define MAPLANG_INCLUDE_MAPLANG_SIMPLESOURCE_H_

#include <string>

#include "maplang/IImplementation.h"
#include "maplang/IPacketPusher.h"

namespace maplang {

class SimpleSource : public IImplementation, public ISource {
 public:
  void setPacketPusher(const std::shared_ptr<IPacketPusher>& pusher) override {
    mPusher = pusher;
  }

  IPathable* asPathable() override { return nullptr; }
  ISource* asSource() override { return this; }
  IGroup* asGroup() override { return nullptr; }

  void sendPacket(const Packet& packet, const std::string& fromChannel) {
    mPusher->pushPacket(packet, fromChannel);
  }

 private:
  std::shared_ptr<IPacketPusher> mPusher;
};

}  // maplang

#endif  // MAPLANG_INCLUDE_MAPLANG_SIMPLESOURCE_H_
