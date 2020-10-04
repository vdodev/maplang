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

#include <queue>
#include <stack>

#include "Graph.h"

namespace std {

template <class ItemClass>
struct hash<pair<ItemClass, string>> final {
  size_t operator()(const pair<ItemClass, string>& key) const {
    return hash<decltype(key.first)>()(key.first) ^ hash<decltype(key.second)>()(key.second);
  }
};

template <>
struct hash<const string> final {
  size_t operator()(const string& str) const { return std::hash<string>()(str); }
};

}  // namespace std

namespace maplang {

template <class ItemClass, class EdgeClass>
void Graph<ItemClass, EdgeClass>::visitGraphElements(const GraphElementVisitor& visitor) const {
  std::list<std::shared_ptr<GraphElement<ItemClass, EdgeClass>>> elementList;
  for (const auto& itemMapPair : mItemToGraphElementMap) {
    const auto& pathableIdToElementMap = itemMapPair.second;

    for (const auto& pathableIdElementPair : pathableIdToElementMap) {
      const auto& graphElement = pathableIdElementPair.second;

      visitor(graphElement);
    }
  }
}

template <class ItemClass, class EdgeClass>
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

template <class ItemClass, class EdgeClass>
EdgeClass& Graph<ItemClass, EdgeClass>::connect(
    const ItemClass& fromItem,
    const std::string& fromChannel,
    const ItemClass& toItem,
    const std::string& fromPathableId,
    const std::string& toPathableId) {
  auto fromGraphElement = getOrCreateGraphElement(fromItem, fromPathableId);
  auto toGraphElement = getOrCreateGraphElement(toItem, toPathableId);

  typename GraphElementType::EdgeKey edgeKey {fromChannel, toGraphElement};
  auto edgeIt = fromGraphElement->forwardEdges.find(edgeKey);
  const bool alreadyConnected = edgeIt != toGraphElement->forwardEdges.end();
  if (alreadyConnected) {
    return edgeIt->second;
  }

  EdgeClass edge;
  edge.otherGraphElement = toGraphElement;

  auto insertedIt = fromGraphElement->forwardEdges.emplace(edgeKey, std::move(edge)).first;

  bool haveBackEdgeAlready = false;
  for (auto it = toGraphElement->backEdges.begin(); it != toGraphElement->backEdges.end();) {
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

template <class ItemClass, class EdgeClass>
void Graph<ItemClass, EdgeClass>::removeItem(const ItemClass& removeItem) {
  auto mapIt = mItemToGraphElementMap.find(removeItem);
  if (mapIt == mItemToGraphElementMap.end()) {
    return;
  }

  // For each DataGraphElement (Node + pathableId, a.k.a. element in pahtableIdToElementMap), remove forward and back
  // connections.

  auto& pathableIdToElementMap = mapIt->second;
  for (auto& pathableElementPair : pathableIdToElementMap) {
    const std::string& removedNodeCurrentPathableId = pathableElementPair.first;
    std::shared_ptr<GraphElementType> element = pathableElementPair.second;

    // Remove forward edges
    for (const auto& edgePair : element->forwardEdges) {
      const auto& edge = edgePair.second;
      const std::string& toPathableId = edge.otherGraphElement->pathableId;

      disconnect(element->item, edge.channel, edge.otherGraphElement->item, removedNodeCurrentPathableId, toPathableId);
    }

    // Removing incoming edges
    for (const auto& weakPreviousElement : element->backEdges) {
      auto previousElement = weakPreviousElement.lock();
      if (previousElement == nullptr) {
        continue;
      }

      for (auto& candidateIncomingEdgePair : previousElement->forwardEdges) {
        const auto& edge = candidateIncomingEdgePair.second;
        if (!(edge.otherGraphElement->item == removeItem)) {
          continue;
        }

        disconnect(
            previousElement->item,
            edge.channel,
            edge.otherGraphElement->item,
            previousElement->pathableId,
            removedNodeCurrentPathableId);
      }
    }
  }

  mItemToGraphElementMap.erase(mapIt);
}

template <class ItemClass, class EdgeClass>
void Graph<ItemClass, EdgeClass>::disconnect(
    const ItemClass& fromNode,
    const std::string& fromChannel,
    const ItemClass& toNode,
    const std::string& fromPathableId,
    const std::string& toPathableId) {
  if (!hasItem(fromNode, fromPathableId) || !hasItem(toNode, toPathableId)) {
    return;
  }

  auto fromGraphElement = getOrCreateGraphElement(fromNode, fromPathableId);
  auto toGraphElement = getOrCreateGraphElement(toNode, toPathableId);

  // Remove forward edges
  auto& forwardEdges = fromGraphElement->forwardEdges;
  typename GraphElementType::EdgeKey edgeKey;
  edgeKey.channel = fromChannel;
  edgeKey.toGraphElement = toGraphElement;
  auto forwardEdgeIt = forwardEdges.find(edgeKey);
  if (forwardEdgeIt == forwardEdges.end()) {
    return;
  }

  for (auto it = toGraphElement->backEdges.begin(); it != toGraphElement->backEdges.end();) {
    auto oldIt = it;
    it++;

    const auto strongBackEdge = oldIt->lock();
    if (strongBackEdge == nullptr || strongBackEdge == fromGraphElement) {
      toGraphElement->backEdges.erase(oldIt);
    }
  }

  // Erase the forward edge last because it contains the shared_ptr, while back-edges are weak_ptrs.
  forwardEdges.erase(edgeKey);
}

template <class ItemClass, class EdgeClass>
bool Graph<ItemClass, EdgeClass>::hasItem(const ItemClass& item, const std::string& pathableId) const {
  auto key = make_pair(item, pathableId);
  auto mapIt = mItemToGraphElementMap.find(item);
  if (mapIt == mItemToGraphElementMap.end()) {
    return false;
  }

  auto& pathableIdToElementMap = mapIt->second;
  return pathableIdToElementMap.find(pathableId) != pathableIdToElementMap.end();
}

template <class ItemClass, class EdgeClass>
bool Graph<ItemClass, EdgeClass>::hasItemWithAnyPathableId(const ItemClass& item) const {
  return mItemToGraphElementMap.find(item) != mItemToGraphElementMap.end();
}

template <class ItemClass, class EdgeClass>
std::shared_ptr<GraphElement<ItemClass, EdgeClass>> Graph<ItemClass, EdgeClass>::getOrCreateGraphElement(
    const ItemClass& item,
    const std::string& pathableId) {
  auto mapIt = mItemToGraphElementMap.find(item);
  if (mapIt == mItemToGraphElementMap.end()) {
    auto insertedIt = mItemToGraphElementMap.emplace(
        std::make_pair(item, std::unordered_map<std::string, std::shared_ptr<GraphElementType>>()));

    mapIt = insertedIt.first;
  }

  auto& pathableIdToElementMap = mapIt->second;
  auto elementIt = pathableIdToElementMap.find(pathableId);
  if (elementIt != pathableIdToElementMap.end()) {
    return elementIt->second;
  }

  auto graphElement = std::make_shared<GraphElement<ItemClass, EdgeClass>>(item, pathableId);

  pathableIdToElementMap[pathableId] = graphElement;

  return graphElement;
}

}  // namespace maplang
