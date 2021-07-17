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

#include "maplang/Graph.h"

#include <queue>
#include <sstream>
#include <stack>

using namespace std;
using namespace nlohmann;

namespace maplang {

void Graph::visitGraphElements(const GraphElementVisitor& visitor) const {
  list<shared_ptr<GraphElement>> elementList;
  for (const auto& elementMapPair : mNameToElementMap) {
    const auto& graphElement = elementMapPair.second;
    visitor(graphElement);
  }
}

void Graph::visitGraphElementsHeadsLast(
    const GraphElementVisitor& visitor) const {
  queue<shared_ptr<GraphElement>> toProcess;
  queue<shared_ptr<GraphElement>> heads;

  visitGraphElements(
      [&heads, &visitor](const shared_ptr<GraphElement>& graphElement) {
        if (graphElement->backEdges.empty()) {
          heads.push(graphElement);
          return;
        }

        visitor(graphElement);
      });

  while (!heads.empty()) {
    const auto pathableElement = move(heads.front());
    heads.pop();
    visitor(pathableElement);
  }
}

GraphEdge& Graph::connect(
    const string& fromElementName,
    const string& fromChannel,
    const string& toElementName) {
  const shared_ptr<GraphElement> fromElement =
      getOrCreateGraphElement(fromElementName);

  const shared_ptr<GraphElement> toElement =
      getOrCreateGraphElement(toElementName);

  std::vector<GraphEdge>& channelEdges =
      fromElement->forwardEdges.try_emplace(fromChannel).first->second;
  for (auto& edge : channelEdges) {
    if (edge.next == toElement) {
      return edge;
    }
  }

  GraphEdge edge;
  edge.next = toElement;
  edge.channel = fromChannel;

  channelEdges.emplace_back(move(edge));

  bool haveBackEdgeAlready = false;
  for (auto it = toElement->backEdges.begin();
       it != toElement->backEdges.end();) {
    auto oldIt = it;
    it++;

    const auto strongBackEdge = oldIt->lock();
    if (strongBackEdge == nullptr) {
      toElement->backEdges.erase(oldIt);
    } else if (strongBackEdge == fromElement) {
      haveBackEdgeAlready = true;
    }
  }

  if (!haveBackEdgeAlready) {
    toElement->backEdges.push_back(fromElement);
  }

  return *channelEdges.rbegin();
}

void Graph::disconnect(
    const string& fromElementName,
    const string& fromChannel,
    const string& toElementName) {
  if (!hasElement(fromElementName) || !hasElement(fromElementName)) {
    return;
  }

  const auto fromElement = getOrCreateGraphElement(fromElementName);
  const auto toElement = getOrCreateGraphElement(toElementName);

  // Remove forward edges
  auto& forwardEdges = fromElement->forwardEdges;
  auto forwardEdgeIt = forwardEdges.find(fromChannel);
  if (forwardEdgeIt == forwardEdges.end()) {
    return;
  }

  for (auto it = toElement->backEdges.begin();
       it != toElement->backEdges.end();) {
    auto oldIt = it;
    it++;

    const auto strongBackEdge = oldIt->lock();
    if (strongBackEdge == nullptr || strongBackEdge == fromElement) {
      toElement->backEdges.erase(oldIt);
    }
  }

  // Erase the forward edge last because it contains the shared_ptr, while
  // back-edges are weak_ptrs.
  forwardEdges.erase(fromChannel);
}

bool Graph::hasElement(const string& elementName) const {
  auto mapIt = mNameToElementMap.find(elementName);
  return mapIt != mNameToElementMap.end();
}

shared_ptr<GraphElement> Graph::getOrCreateGraphElement(
    const string& elementName) {
  auto it = mNameToElementMap.find(elementName);

  if (it != mNameToElementMap.end()) {
    return it->second;
  }

  auto insertedIt = mNameToElementMap.emplace(
      make_pair(elementName, make_shared<GraphElement>(elementName)));

  it = insertedIt.first;

  const auto graphElement = it->second;

  return graphElement;
}

optional<shared_ptr<GraphElement>> Graph::getGraphElement(
    const string& elementName) const {
  auto it = mNameToElementMap.find(elementName);

  if (it == mNameToElementMap.end()) {
    return {};
  }

  return it->second;
}

GraphElement::GraphElement(const std::string& elementName)
    : elementName(elementName) {}

void GraphElement::cleanUpEmptyEdges() {
  for (auto it = forwardEdges.begin(); it != forwardEdges.end();) {
    auto currIt = it;
    it++;

    auto& channelEdges = currIt->second;
    if (channelEdges.empty()) {
      forwardEdges.erase(currIt);
    }
  }

  for (auto it = backEdges.begin(); it != backEdges.end();) {
    auto currIt = it;
    it++;

    if (currIt->lock() == nullptr) {
      backEdges.erase(currIt);
    }
  }
}

}  // namespace maplang
