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

void Graph::visitNodes(const NodeVisitor& visitor) const {
  list<shared_ptr<GraphNode>> nodeList;
  for (const auto& nodeMapPair : mNameToNodeMap) {
    const auto& node = nodeMapPair.second;
    visitor(node);
  }
}

void Graph::visitNodesHeadsLast(const NodeVisitor& visitor) const {
  queue<shared_ptr<GraphNode>> toProcess;
  queue<shared_ptr<GraphNode>> heads;

  visitNodes([&heads, &visitor](const shared_ptr<GraphNode>& node) {
    if (node->backEdges.empty()) {
      heads.push(node);
      return;
    }

    visitor(node);
  });

  while (!heads.empty()) {
    const auto node = move(heads.front());
    heads.pop();
    visitor(node);
  }
}

GraphEdge& Graph::connect(
    const string& fromNodeName,
    const string& fromChannel,
    const string& toNodeName) {
  const shared_ptr<GraphNode> fromNode = *getNode(fromNodeName);
  const shared_ptr<GraphNode> toNode = *getNode(toNodeName);
  std::vector<GraphEdge>& channelEdges =
      fromNode->forwardEdges.try_emplace(fromChannel).first->second;
  for (auto& edge : channelEdges) {
    if (edge.next == toNode) {
      return edge;
    }
  }

  GraphEdge edge;
  edge.next = toNode;
  edge.channel = fromChannel;

  channelEdges.emplace_back(move(edge));

  bool haveBackEdgeAlready = false;
  for (auto it = toNode->backEdges.begin(); it != toNode->backEdges.end();) {
    auto oldIt = it;
    it++;

    const auto strongBackEdge = oldIt->lock();
    if (strongBackEdge == nullptr) {
      toNode->backEdges.erase(oldIt);
    } else if (strongBackEdge == fromNode) {
      haveBackEdgeAlready = true;
    }
  }

  if (!haveBackEdgeAlready) {
    toNode->backEdges.push_back(fromNode);
  }

  return *channelEdges.rbegin();
}

void Graph::disconnect(
    const string& fromNodeName,
    const string& fromChannel,
    const string& toNodeName) {
  if (!hasNode(fromNodeName) || !hasNode(fromNodeName)) {
    return;
  }

  const auto fromNode = *getNode(fromNodeName);
  const auto toNode = *getNode(toNodeName);

  // Remove forward edges
  auto& forwardEdges = fromNode->forwardEdges;
  auto forwardEdgeIt = forwardEdges.find(fromChannel);
  if (forwardEdgeIt == forwardEdges.end()) {
    return;
  }

  for (auto it = toNode->backEdges.begin(); it != toNode->backEdges.end();) {
    auto oldIt = it;
    it++;

    const auto strongBackEdge = oldIt->lock();
    if (strongBackEdge == nullptr || strongBackEdge == fromNode) {
      toNode->backEdges.erase(oldIt);
    }
  }

  // Erase the forward edge last because it contains the shared_ptr, while
  // back-edges are weak_ptrs.
  forwardEdges.erase(fromChannel);
}

bool Graph::hasNode(const string& nodeName) const {
  auto mapIt = mNameToNodeMap.find(nodeName);
  return mapIt != mNameToNodeMap.end();
}

shared_ptr<GraphNode> Graph::createGraphNode(
    const string& nodeName,
    bool allowIncomingConnections,
    bool allowOutgoingConnections) {
  auto it = mNameToNodeMap.find(nodeName);

  if (it != mNameToNodeMap.end()) {
    throw runtime_error(
        "Cannot create GraphNode '" + nodeName + "'. It already exists.");
  }

  auto insertedIt = mNameToNodeMap.emplace(make_pair(
      nodeName,
      make_shared<GraphNode>(
          nodeName,
          allowIncomingConnections,
          allowOutgoingConnections)));

  it = insertedIt.first;

  return it->second;
}

optional<shared_ptr<GraphNode>> Graph::getNode(const string& nodeName) const {
  auto it = mNameToNodeMap.find(nodeName);

  if (it == mNameToNodeMap.end()) {
    return {};
  }

  return it->second;
}

shared_ptr<GraphNode> Graph::getNodeOrThrow(const string& nodeName) const {
  auto it = mNameToNodeMap.find(nodeName);

  if (it == mNameToNodeMap.end()) {
    throw runtime_error(
        "Cannot get GraphNode '" + nodeName + "'. It does not exist.");
  }

  return it->second;
}

}  // namespace maplang
