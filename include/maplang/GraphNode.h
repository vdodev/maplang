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

#ifndef MAPLANG_INCLUDE_MAPLANG_GRAPHNODE_H_
#define MAPLANG_INCLUDE_MAPLANG_GRAPHNODE_H_

#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

#include "maplang/GraphEdge.h"
#include "maplang/IPacketPusher.h"

namespace maplang {

struct GraphNode final {
 public:
  GraphNode(
      const std::string& elementName,
      bool allowIncomingConnections,
      bool allowOutgoingConnections);

  void cleanUpEmptyEdges();

 public:
  const std::string name;
  const bool allowsIncomingConnections;
  const bool allowsOutgoingConnections;

  std::string instanceName;
  std::shared_ptr<IPacketPusher> packetPusher;

  // for parameter propagation when the downstream node(s) get a packet from
  // this node.
  std::shared_ptr<const nlohmann::json> lastReceivedParameters;
  std::list<std::weak_ptr<GraphNode>> backEdges;

  // All GraphElements this one connects to. channel => edges from this channel
  std::unordered_map<std::string, std::vector<GraphEdge>> forwardEdges;
};

}  // namespace maplang

#endif  // MAPLANG_INCLUDE_MAPLANG_GRAPHNODE_H_
