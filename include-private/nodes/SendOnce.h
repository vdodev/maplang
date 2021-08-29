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

#ifndef MAPLANG_SEND_ONCE_H_
#define MAPLANG_SEND_ONCE_H_

#include "maplang/Factories.h"
#include "maplang/IImplementation.h"
#include "maplang/ISource.h"

namespace maplang {

class SendOnce : public IImplementation, public ISource {
 public:
  SendOnce(const Factories& factories, const nlohmann::json& sendOnceData);
  ~SendOnce() override = default;

  void setPacketPusher(const std::shared_ptr<IPacketPusher>& pusher) override;

  IPathable* asPathable() override { return nullptr; }
  ISource* asSource() override { return this; }
  IGroup* asGroup() override { return nullptr; }

 private:
  const Factories mFactories;
  const nlohmann::json mSendOnceData;
};

}  // namespace maplang

#endif  // MAPLANG_SEND_ONCE_H_
