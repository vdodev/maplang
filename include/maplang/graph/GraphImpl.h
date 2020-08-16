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

#include "Graph.h"
#include <stack>
#include <queue>

namespace std {

template<class ItemClass>
struct hash<pair<ItemClass, string>> final {
  size_t operator()(const pair<ItemClass, string>& key) const {
    return hash<decltype(key.first)>()(key.first) ^
        hash<decltype(key.second)>()(key.second);
  }
};

template<>
struct hash<const string> final {
  size_t operator()(const string& str) const {
    return std::hash<string>()(str);
  }
};

}  // namespace std

namespace maplang {

template<class ItemClass, class EdgeClass>
void Graph<ItemClass, EdgeClass>::visitGraphElements(const GraphElementVisitor& visitor) const {
  std::list<std::shared_ptr<GraphElement<ItemClass, EdgeClass>>> elementList;
  for (const auto& pair : mItemToGraphElementMap) {
    const auto& graphElement = pair.second;

    visitor(graphElement);
  }
}

template<class ItemClass, class EdgeClass>
void Graph<ItemClass, EdgeClass>::visitGraphElementsHeadsLast(const GraphElementVisitor& visitor) const {
  std::queue<std::shared_ptr<GraphElement<std::string>>> toProcess;
  std::queue<std::shared_ptr<GraphElementType>> heads;

  visitGraphElements([&heads, &visitor](const std::shared_ptr<GraphElementType>& graphElement) {
    if (graphElement->backEdges.empty()) {
      heads.push(graphElement);
      return;
    }

    visitor(graphElement);
  });

  while (!heads.empty()) {
    const auto graphElement = move(heads.front());
    heads.pop();
    visitor(graphElement);
  }
}

template<class ItemClass, class EdgeClass>
EdgeClass& Graph<ItemClass, EdgeClass>::connect(
    const ItemClass& fromItem,
    const std::string& fromChannel,
    const ItemClass& toItem,
    const std::string& fromPathableId,
    const std::string& toPathableId) {
  auto fromGraphElement = getOrCreateGraphElement(fromItem, fromPathableId);
  auto toGraphElement = getOrCreateGraphElement(toItem, toPathableId);

  typename GraphElementType::EdgeKey edgeKey{fromChannel, toGraphElement};
  auto edgeIt = fromGraphElement->forwardEdges.find(edgeKey);
  const bool alreadyConnected = edgeIt != toGraphElement->forwardEdges.end();
  if (alreadyConnected) {
    return edgeIt->second;
  }

  EdgeClass edge;
  edge.otherGraphElement = toGraphElement;

  auto insertedIt = fromGraphElement->forwardEdges.emplace(edgeKey, std::move(edge)).first;

  bool haveBackEdgeAlready = false;
  for (auto it = toGraphElement->backEdges.begin(); it != toGraphElement->backEdges.end(); ) {
    auto oldIt = it;
    it++;

    const auto strongBackEdge = oldIt->lock();
    if (strongBackEdge == nullptr) {
      toGraphElement->backEdges.erase(oldIt);
    } else if (strongBackEdge == fromGraphElement) {
      haveBackEdgeAlready = true;
    }
  }

  if (!haveBackEdgeAlready) {
    toGraphElement->backEdges.push_back(fromGraphElement);
  }

  return insertedIt->second;
}

template <typename T>
static void removeFromListIfPresent(std::list<T>* removeFromList,
                                    const T& itemToRemove) {
  for (auto it = removeFromList->begin(); it != removeFromList->end(); it++) {
    if (*it == itemToRemove) {
      removeFromList->erase(it);
      return;
    }
  }
}

template<class ItemClass, class EdgeClass>
void Graph<ItemClass, EdgeClass>::disconnect(
    const ItemClass& fromNode,
    const std::string& fromChannel,
    const ItemClass& toNode,
    const std::string& fromPathableId,
    const std::string& toPathableId) {
  if (!hasNode(fromNode, fromPathableId) ||
      !hasNode(toNode, toPathableId)) {
    return;
  }

  auto fromGraphElement = getOrCreateGraphElement(fromNode, fromPathableId);
  auto toGraphElement = getOrCreateGraphElement(toNode, toPathableId);

  removeFromListIfPresent(&fromGraphElement->forwardEdges, toGraphElement);

  for (auto it = toGraphElement->backEdges.begin(); it != toGraphElement->backEdges.end(); ) {
    auto oldIt = it;
    it++;

    const auto strongBackEdge = oldIt->lock();
    if (strongBackEdge == nullptr || strongBackEdge == fromGraphElement) {
      toGraphElement->backEdges.erase(oldIt);
    }
  }
}

template<class ItemClass, class EdgeClass>
bool Graph<ItemClass, EdgeClass>::hasItem(const ItemClass& item, const std::string& pathableId) const {
  auto key = make_pair(item, pathableId);
  return mItemToGraphElementMap.find(key) != mItemToGraphElementMap.end();
}

template<class ItemClass, class EdgeClass>
std::shared_ptr<GraphElement<ItemClass, EdgeClass>> Graph<ItemClass, EdgeClass>::getOrCreateGraphElement(
    const ItemClass& item, const std::string& pathableId) {
  GraphElementLookupKey key = make_pair(item, pathableId);
  auto it = mItemToGraphElementMap.find(key);

  if (it != mItemToGraphElementMap.end()) {
    return it->second;
  }

  auto graphElement = std::make_shared<GraphElement<ItemClass, EdgeClass>>(item, pathableId);

  mItemToGraphElementMap[key] = graphElement;

  return graphElement;
}

}  // namespace maplang
