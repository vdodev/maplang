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

#ifndef __MAPLANG_IGROUP_H__
#define __MAPLANG_IGROUP_H__

#include <memory>

#include "maplang/IImplementation.h"

namespace maplang {

class IGroup {
 public:
  virtual ~IGroup() = default;

  virtual size_t getInterfaceCount() = 0;
  virtual std::string getInterfaceName(size_t nodeIndex) = 0;

  virtual std::shared_ptr<IImplementation> getInterface(const std::string& nodeName) = 0;
};

}  // namespace maplang

#endif  // __MAPLANG_IGROUP_H__