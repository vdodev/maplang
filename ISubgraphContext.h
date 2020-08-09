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

#ifndef __MAPLANG_ISUBGRAPHCONTEXT_H__
#define __MAPLANG_ISUBGRAPHCONTEXT_H__

#include <memory>

#include "json.hpp"

typedef struct uv_loop_s uv_loop_t;

namespace maplang {

class INode;

class ISubgraphContext {
 public:
  virtual std::shared_ptr<uv_loop_t> getUvLoop() const = 0;
};

}  // namespace maplang

#endif  // __MAPLANG_ISUBGRAPHCONTEXT_H__
