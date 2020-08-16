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
#include <list>
#include "maplang/graph/DefaultGraphEdge.h"

namespace maplang {

template<class ItemClass, class EdgeClass = DefaultGraphEdge<ItemClass>>
struct GraphElement final {
 public:
  using GraphElementType = GraphElement<ItemClass, EdgeClass>;

  struct EdgeKey {
    std::string channel;
    std::shared_ptr<GraphElementType> toGraphElement;

    bool operator==(const EdgeKey& other) const {
      return channel == other.channel && toGraphElement == other.toGraphElement;
    }
  };

 private:
  struct EdgeKeyHasher {
    std::size_t operator()(const GraphElementType::EdgeKey& key) const {
      return std::hash<decltype(key.channel)>()(key.channel) ^ std::hash<decltype(key.toGraphElement)>()(key.toGraphElement);
    }
  };

 public:
  GraphElement(const ItemClass& item, const std::string& pathableId) : item(item), pathableId(pathableId) {}

  ItemClass item;
  std::list<std::weak_ptr<GraphElementType>> backEdges;
  std::unordered_map<EdgeKey, EdgeClass, EdgeKeyHasher> forwardEdges;  // All GraphElements this one connects to.
  std::string pathableId;
};

}  // namespace maplang

#endif  // MAPLANG_GRAPHELEMENT_H_
