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

#ifndef __MAPLANG_ISOURCE_H__
#define __MAPLANG_ISOURCE_H__

#include <memory>
#include <vector>

#include "IPacketPusher.h"
#include "ISink.h"
#include "Packet.h"

namespace maplang {

class IPacketPusher;

class ISource {
 public:
  virtual ~ISource() = default;

  /**
   * Sets the pusher this ISource uses to push packets into the Graph.
   *
   * @param output When a Packet is ready, push witih pusher->pushPacket().
   */
  virtual void setPacketPusher(
      const std::shared_ptr<IPacketPusher>& pusher) = 0;
};

}  // namespace maplang

#endif  // __MAPLANG_ISOURCE_H__
