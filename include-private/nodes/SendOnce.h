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

#ifndef __MAPLANG_SEND_ONCE_H__
#define __MAPLANG_SEND_ONCE_H__

#include "maplang/INode.h"
#include "maplang/ISource.h"

namespace maplang {

class SendOnce : public INode, public ISource {
 public:
  SendOnce(const nlohmann::json& sendOnceData);
  ~SendOnce() override = default;

  void setPacketPusher(const std::shared_ptr<IPacketPusher>& pusher) override;

  IPathable *asPathable() override { return nullptr; }
  ISink *asSink() override { return nullptr; }
  ISource *asSource() override { return this; }
  ICohesiveGroup* asGroup() override { return nullptr; }

 private:
  const nlohmann::json mSendOnceData;
};

}  // namespace maplang

#endif  // __MAPLANG_SEND_ONCE_H__
