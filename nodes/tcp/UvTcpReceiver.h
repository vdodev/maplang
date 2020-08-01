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

#include "../../INode.h"
#include "../../ISource.h"

typedef struct uv_loop_s uv_loop_t;

namespace dgraph {

class UvTcpReceiver final : public INode, public ISource {
 public:
  ~UvTcpReceiver() override = default;

  void setPacketPusher(const std::shared_ptr<IPacketPusher>& pusher) override;

  IPathable* asPathable() override { return nullptr; }
  ISource* asSource() override { return this; }
  ISink* asSink() override { return nullptr; }

 private:
  std::shared_ptr<IPacketPusher> mPusher;
  std::shared_ptr<uv_loop_t> mUvLoop;
};

}  // namespace dgraph
