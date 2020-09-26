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

#ifndef __MAPLANG_GRAPH_H_
#define __MAPLANG_GRAPH_H_

#include <list>
#include <functional>

#include "maplang/graph/DefaultGraphEdge.h"
#include "maplang/graph/GraphElement.h"
#include "maplang/INode.h"

namespace maplang {

template<class ItemClass, class EdgeClass = DefaultGraphEdge<ItemClass>>
class Graph final {
 public:
  using GraphElementType = GraphElement<ItemClass, EdgeClass>;

  EdgeClass& connect(
      const ItemClass& fromItem,
      const std::string& fromChannel,
      const ItemClass& toItem,
      const std::string& fromPathableId = "",
      const std::string& toPathableId = "");

  void disconnect(
      const ItemClass& fromItem,
      const std::string& fromChannel,
      const ItemClass& toItem,
      const std::string& fromPathableId = "",
      const std::string& toPathableId = "");

  void removeItem(const ItemClass& item);

  using GraphElementVisitor = std::function<void (const std::shared_ptr<GraphElementType>& graphElement)>;

  /**
   * Visits GraphElements. Order is unspecified.
   * @param visitor Called for each GraphElement in the Graph.
   */
  void visitGraphElements(const GraphElementVisitor& visitor) const;

  void visitGraphElementsHeadsLast(const GraphElementVisitor& visitor) const;

  bool hasItem(const ItemClass& item, const std::string& pathableId) const;
  bool hasItemWithAnyPathableId(const ItemClass& item) const;
  std::shared_ptr<GraphElementType> getOrCreateGraphElement(const ItemClass& item, const std::string& pathableId);

 private:
  // Item -> Pathable ID -> Graph Element
  std::unordered_map<const ItemClass, std::unordered_map<std::string, std::shared_ptr<GraphElementType>>> mItemToGraphElementMap;
};

}  // namespace maplang

#include "maplang/graph/GraphImpl.h"

#endif // __MAPLANG_GRAPH_H_
