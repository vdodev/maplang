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

#ifndef MAPLANG_SRC_LAMBDAPATHABLE_H_
#define MAPLANG_SRC_LAMBDAPATHABLE_H_

#include <maplang/IImplementation.h>

#include <functional>
#include <iostream>

namespace maplang {

class LambdaPathable : public IImplementation, public IPathable {
 public:
  explicit LambdaPathable(
      std::function<void(const PathablePacket& packet)>&& onPacket)
      : mOnPacket(move(onPacket)) {}

  void handlePacket(const PathablePacket& packet) override {
    mOnPacket(packet);
  }

  IPathable* asPathable() override { return this; }
  ISource* asSource() override { return nullptr; }
  IGroup* asGroup() override { return nullptr; }

 private:
  std::function<void(const PathablePacket& packet)> mOnPacket;
};

}  // namespace maplang

#endif  // MAPLANG_SRC_LAMBDAPATHABLE_H_
