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

using namespace std;
using namespace nlohmann;

namespace dgraph {

void Graph::visitGraphElements(const GraphElementVisitor& visitor) const {
  std::list<std::shared_ptr<GraphElement>> elementList;
  for (const auto& pair : mNodeToGraphElementMap) {
    const string& pathableId = pair.first.second;
    const auto& graphElement = pair.second;

    visitor(pathableId, graphElement);
  }
}

void Graph::connect(
    const shared_ptr<INode>& fromNode,
    const string& fromChannel,
    const shared_ptr<INode>& toNode,
    const string& fromPathableId,
    const string& toPathableId) {

  const bool firstTimeFromNodeWasAdded = !hasNode(fromNode, fromPathableId);
  const auto fromPathable = fromNode->asPathable();
  const auto source = fromNode->asSource();
  const auto toPathable = toNode->asPathable();
  const auto sink = toNode->asSink();

  if (!source && !fromPathable) {
    throw runtime_error(
        "Cannot make a graph connection starting from a Node which is not a "
        "source or a pathable.");
  }

  if (!sink && !toPathable) {
    throw runtime_error(
        "Cannot make a graph connection ending with a Node which is not a sink "
        "or a pathable.");
  }

  auto fromGraphElement = getOrCreateGraphElement(fromNode, fromPathableId);
  auto toGraphElement = getOrCreateGraphElement(toNode, toPathableId);

  Edge forwardEdge;
  forwardEdge.channel = fromChannel;
  forwardEdge.otherGraphElement = toGraphElement;

  fromGraphElement->forwardEdges.push_back(forwardEdge);
  toGraphElement->backEdges.push_back(fromGraphElement);
}

void Graph::validateNodeTypesAreCompatible(const shared_ptr<INode>& node) const {
  const auto pathable = node->asPathable();
  const auto sink = node->asSink();
  const auto source = node->asSource();

  const bool isSink = sink != nullptr;
  const bool isSource = source != nullptr;
  const bool isPathable = pathable != nullptr;

  /*
   * Cases
   *
   * sink + source - ok, source gets the pusher when connected to the graph.
   * sink + none - ok. Won't ever get a packet pusher, but a sink without an
   * exit source is a thing. The graph ends somewhere. none + source - ok,
   * source gets a pusher when connected to the graph. However it produces
   * packets is up to it. not a sink, source, or pathable - not ok. it can't be
   * used - no way to send or get packets.
   *
   * pathable + source or sink - not ok. A pathable is basically a sink and
   * source combined.
   */

  if (isPathable && (isSource || isSink)) {
    throw runtime_error("A pathable cannot be a source or a sink.");
  } else if (!isPathable && !isSource && !isSink) {
    throw runtime_error("A node must be a pathable, sink or source.");
  }
}

static void removeFromEdgeListIfPresent(
    list<Edge>* edgeList, const shared_ptr<GraphElement>& otherGraphElement,
    const string& otherChannel) {
  for (auto it = edgeList->begin(); it != edgeList->end(); it++) {
    if (it->otherGraphElement == otherGraphElement &&
        it->channel == otherChannel) {
      edgeList->erase(it);
      return;
    }
  }
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

void Graph::disconnect(
    const shared_ptr<INode>& fromNode,
    const string& fromChannel,
    const shared_ptr<INode>& toNode,
    const string& fromPathableId,
    const string& toPathableId) {
  if (!hasNode(fromNode, fromPathableId) ||
      !hasNode(toNode, toPathableId)) {
    return;
  }

  auto fromGraphElement = getOrCreateGraphElement(fromNode, fromPathableId);
  auto toGraphElement = getOrCreateGraphElement(toNode, toPathableId);

  removeFromEdgeListIfPresent(&fromGraphElement->forwardEdges, toGraphElement,
                              fromChannel);
  removeFromListIfPresent(&toGraphElement->backEdges, fromGraphElement);
}

bool Graph::hasNode(const shared_ptr<INode>& node, const string& pathableId) const {
  auto key = make_pair(node.get(), pathableId);
  return mNodeToGraphElementMap.find(key) != mNodeToGraphElementMap.end();
}

shared_ptr<GraphElement> Graph::getOrCreateGraphElement(const shared_ptr<INode>& node, const string& pathableId) {
  string usePathableId = pathableId;

  validateNodeTypesAreCompatible(node);

  if (!node->asPathable()) {
    usePathableId = "";
  }

  auto key = make_pair(node.get(), usePathableId);
  auto it = mNodeToGraphElementMap.find(key);

  if (it != mNodeToGraphElementMap.end()) {
    return it->second;
  }

  auto graphElement = make_shared<GraphElement>();
  graphElement->node = node;

  mNodeToGraphElementMap[key] = graphElement;

  return graphElement;
}

}  // namespace dgraph
