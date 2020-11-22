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

#ifndef __MAPLANG_DEFAULTGRAPHEDGE_H_
#define __MAPLANG_DEFAULTGRAPHEDGE_H_

#include <memory>
#include <string>

namespace maplang {

template <class ItemClass, class EdgeClass, class GraphElementInfo>
struct GraphElement;

template <class ItemClass>
struct DefaultGraphEdge {
  using UnusedExtraInfoType = std::shared_ptr<void>;
  std::shared_ptr<
      GraphElement<ItemClass, DefaultGraphEdge<ItemClass>, UnusedExtraInfoType>>
      otherGraphElement;
};

}  // namespace maplang

#endif  // __MAPLANG_DEFAULTGRAPHEDGE_H_
