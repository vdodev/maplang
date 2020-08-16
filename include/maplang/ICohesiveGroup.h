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

#ifndef __MAPLANG_ICOHESIVEGROUP_H__
#define __MAPLANG_ICOHESIVEGROUP_H__

#include "INode.h"

namespace maplang {

class ICohesiveGroup {
 public:
  virtual ~ICohesiveGroup() = default;

  virtual size_t getNodeCount() = 0;
  virtual std::string getNodeName(size_t nodeIndex) = 0;

  virtual std::shared_ptr<INode> getNode(const std::string& nodeName) = 0;
};

}  // namespace maplang

#endif  // __MAPLANG_ICOHESIVE_NODE_H__
