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

#ifndef MAPLANG_IIMPLEMENTATION_H__
#define MAPLANG_IIMPLEMENTATION_H__

#include <functional>

#include "maplang/IPathable.h"
#include "maplang/ISource.h"
#include "maplang/ISubgraphContext.h"
#include "maplang/json.hpp"

namespace maplang {

class IGroup;

class IImplementation {
 public:
  virtual ~IImplementation() {}

  virtual void setSubgraphContext(
      const std::shared_ptr<ISubgraphContext>& context) {}

  virtual IPathable* asPathable() = 0;
  virtual ISource* asSource() = 0;
  virtual IGroup* asGroup() = 0;
};

}  // namespace maplang

#endif  // MAPLANG_IIMPLEMENTATION_H__
