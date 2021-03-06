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

#ifndef __MAPLANG_ISINK_H__
#define __MAPLANG_ISINK_H__

#include "maplang/Packet.h"

namespace maplang {

class ISink {
 public:
  virtual ~ISink() = default;

  virtual void handlePacket(const Packet& incomingPacket) = 0;
};

class ISinkFactory {
 public:
  virtual ~ISinkFactory() = default;

  virtual std::shared_ptr<ISink> create(
      const nlohmann::json& initParameters) = 0;
};

}  // namespace maplang

#endif  // __MAPLANG_ISINK_H__
