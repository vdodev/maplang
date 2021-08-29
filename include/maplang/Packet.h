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

#ifndef MAPLANG_PACKET_H__
#define MAPLANG_PACKET_H__

#include "maplang/Buffer.h"
#include "maplang/ISubgraphContext.h"
#include "maplang/json.hpp"

namespace maplang {

struct Packet {
  nlohmann::json parameters;
  std::vector<Buffer> buffers;
};

}  // namespace maplang

#endif  // MAPLANG_PACKET_H__
