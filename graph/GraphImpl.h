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

using namespace std;
using namespace nlohmann;

namespace maplang {

template<class ItemClass, class EdgeClass>
void Graph<ItemClass, EdgeClass>::visitGraphElements(const GraphElementVisitor& visitor) const {
  std::list<std::shared_ptr<GraphElement<ItemClass, EdgeClass>>> elementList;
  for (const auto& pair : mItemToGraphElementMap) {
    const string& pathableId = pair.first.second;
    const auto& graphElement = pair.second;

    visitor(pathableId, graphElement);
  }
}

template<class ItemClass, class EdgeClass>
EdgeClass& Graph<ItemClass, EdgeClass>::connect(
    const ItemClass& fromItem,
    const std::string& fromChannel,
    const ItemClass& toItem,
    const string& fromPathableId,
    const string& toPathableId) {
  auto fromGraphElement = getOrCreateGraphElement(fromItem, fromPathableId);
  auto toGraphElement = getOrCreateGraphElement(toItem, toPathableId);

  typename GraphElementType::MapKeyType edgeKey(fromChannel, toGraphElement);
  auto edgeIt = fromGraphElement->forwardEdges.find(edgeKey);
  const bool alreadyConnected = edgeIt != toGraphElement->forwardEdges.end();
  if (alreadyConnected) {
    return edgeIt->second;
  }

  EdgeClass edge;
  edge.otherGraphElement = toGraphElement;
  auto insertedIt = fromGraphElement->forwardEdges.emplace(edgeKey, move(edge)).first;

  return insertedIt->second;
}

template <typename T>
static void removeFromListIfPresent(list<T>* removeFromList,
                                    const T& itemToRemove) {
  for (auto it = removeFromList->begin(); it != removeFromList->end(); it++) {
    if (*it == itemToRemove) {
      removeFromList->erase(it);
      return;
    }
  }
}

template <typename K, typename V>
static void removeFromMapIfPresent(unordered_map<K, V>* fromMap, const K& key) {
  auto it = fromMap->find(key);
  if (it != fromMap->end()) {
    fromMap->erase(it);
  }
}

template<class ItemClass, class EdgeClass>
void Graph<ItemClass, EdgeClass>::disconnect(
    const ItemClass& fromNode,
    const std::string& fromChannel,
    const ItemClass& toNode,
    const string& fromPathableId,
    const string& toPathableId) {
  if (!hasNode(fromNode, fromPathableId) ||
      !hasNode(toNode, toPathableId)) {
    return;
  }

  auto fromGraphElement = getOrCreateGraphElement(fromNode, fromPathableId);
  auto toGraphElement = getOrCreateGraphElement(toNode, toPathableId);

  removeFromListIfPresent(&fromGraphElement->forwardEdges, toGraphElement);
}

template<class ItemClass, class EdgeClass>
bool Graph<ItemClass, EdgeClass>::hasItem(const ItemClass& item, const string& pathableId) const {
  auto key = make_pair(item, pathableId);
  return mItemToGraphElementMap.find(key) != mItemToGraphElementMap.end();
}

template<class ItemClass, class EdgeClass>
shared_ptr<GraphElement<ItemClass, EdgeClass>> Graph<ItemClass, EdgeClass>::getOrCreateGraphElement(const ItemClass& item, const string& pathableId) {
  GraphElementLookupKey key = make_pair(item, pathableId);
  auto it = mItemToGraphElementMap.find(key);

  if (it != mItemToGraphElementMap.end()) {
    return it->second;
  }

  auto graphElement = make_shared<GraphElement<ItemClass, EdgeClass>>(item);

  mItemToGraphElementMap[key] = graphElement;

  return graphElement;
}

}  // namespace maplang
