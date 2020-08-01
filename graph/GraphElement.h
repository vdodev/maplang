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

#ifndef DATA_GRAPH_GRAPH_GRAPHELEMENT_H_
#define DATA_GRAPH_GRAPH_GRAPHELEMENT_H_

#include <list>

#include "../INode.h"
#include "../IPacketPusher.h"
#include "../json.hpp"
#include "Edge.h"

namespace dgraph {

struct GraphElement final {
  std::shared_ptr<INode> node;  // Always set.
  std::shared_ptr<IPacketPusher> pathablePacketPusher;  // Set for IPathables only.
  nlohmann::json lastReceivedParameters; // for parameter propagation the next time we get a packet from this node.
  std::list<std::shared_ptr<GraphElement>> backEdges;  // All nodes that point at this node.
  std::list<Edge> forwardEdges;                        // All nodes this one connects to.
};

}  // namespace dgraph
#endif //DATA_GRAPH_GRAPH_GRAPHELEMENT_H_
