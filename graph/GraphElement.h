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

#ifndef MAPLANG_GRAPHELEMENT_H_
#define MAPLANG_GRAPHELEMENT_H_

#include <unordered_map>
#include "DefaultGraphEdge.h"

namespace maplang {

template<class ItemClass, class EdgeClass = DefaultGraphEdge<ItemClass>>
struct GraphElement final {
 public:
  using MapKeyType = std::pair<std::string, std::shared_ptr<GraphElement<ItemClass, EdgeClass>>>;

 private:
  struct MapKeyHasher {
    std::size_t operator()(const GraphElement<ItemClass, EdgeClass>::MapKeyType& key) const {
      return std::hash<decltype(key.first)>()(key.first) ^ std::hash<decltype(key.second)>()(key.second);
    }
  };

 public:
  GraphElement(const ItemClass& item) : item(item) {}

  ItemClass item;
  std::unordered_map<MapKeyType, EdgeClass, MapKeyHasher> forwardEdges;  // channel => edge: All GraphElements this one connects to.
};

}  // namespace maplang

#endif  // MAPLANG_GRAPHELEMENT_H_
