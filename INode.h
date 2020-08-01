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

#include <functional>

#include "IPathable.h"
#include "ISink.h"
#include "ISource.h"
#include "ISubgraphContext.h"
#include "ISignalBroadcastContext.h"
#include "json.hpp"

namespace dgraph {
class INode {
 public:
  virtual ~INode() {}

  /**
   * NOTE: for the DataGraph to propagate signals and resulting packets
   * correctly, this method must ensure any resulting calls to
   * IPacketPusher::pushPacket() have returned before this method returns.
   *
   * sendSignal() can be used for graph flush operations, graph break-up for
   * scaling, etc. The graph may or may not continue to operate after a signal.
   *
   * @param params
   */
  virtual void sendSignal(const std::shared_ptr<ISignalBroadcastContext> &context, const nlohmann::json &params) {}

  virtual void setSubgraphContext(
      const std::shared_ptr<ISubgraphContext> &context) {}

  virtual IPathable *asPathable() = 0;
  virtual ISink *asSink() = 0;
  virtual ISource *asSource() = 0;
};

}  // namespace dgraph
