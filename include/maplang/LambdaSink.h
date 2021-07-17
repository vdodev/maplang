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

#ifndef MAPLANG_SRC_LAMBDASINK_H_
#define MAPLANG_SRC_LAMBDASINK_H_

#include <maplang/INode.h>

#include <functional>
#include <iostream>

namespace maplang {

class LambdaSink : public INode, public ISink {
 public:
  LambdaSink(std::function<void(const Packet& packet)>&& onPacket)
      : mOnPacket(move(onPacket)) {}

  void handlePacket(const Packet& packet) override {
    std::cout << "Lambda sink got a packet" << std::endl;
    mOnPacket(packet);
  }

  IPathable* asPathable() override { return nullptr; }
  ISink* asSink() override { return this; }
  ISource* asSource() override { return nullptr; }
  ICohesiveGroup* asGroup() override { return nullptr; };

 private:
  std::function<void(const Packet& packet)> mOnPacket;
};

}  // namespace maplang

#endif  // MAPLANG_SRC_LAMBDASINK_H_
