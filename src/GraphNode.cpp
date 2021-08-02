/*
 * Copyright 2020 VDO Dev Inc <support@maplang.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "maplang/GraphNode.h"

namespace maplang {

GraphNode::GraphNode(
    const std::string& _elementName,
    bool _allowIncomingConnections,
    bool _allowOutgoingConnections)
    : name(_elementName), allowsIncomingConnections(_allowIncomingConnections),
      allowsOutgoingConnections(_allowOutgoingConnections) {}

void GraphNode::cleanUpEmptyEdges() {
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
