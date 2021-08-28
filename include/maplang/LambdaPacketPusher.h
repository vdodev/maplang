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

#ifndef MAPLANG_INCLUDE_MAPLANG_LAMBDAPACKETPUSHER_H_
#define MAPLANG_INCLUDE_MAPLANG_LAMBDAPACKETPUSHER_H_

#include <functional>
#include <string>

#include "maplang/IPacketPusher.h"
#include "maplang/Packet.h"

namespace maplang {

class LambdaPacketPusher : public IPacketPusher {
 public:
  LambdaPacketPusher(
      std::function<void(const Packet& packet, const std::string& channel)>&&
          onPacket)
      : mOnPacket(move(onPacket)) {}

  ~LambdaPacketPusher() override = default;

  void pushPacket(const Packet& packet, const std::string& channel) override {
    mOnPacket(packet, channel);
  }

  void pushPacket(Packet&& packet, const std::string& channel) override {
    mOnPacket(packet, channel);
  }

 private:
  std::function<void(const Packet& packet, const std::string& channel)>
      mOnPacket;
};

}  // namespace maplang

#endif  // MAPLANG_INCLUDE_MAPLANG_LAMBDAPACKETPUSHER_H_
