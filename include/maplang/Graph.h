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

#include <functional>
#include <list>

#include "maplang/GraphNode.h"
#include "maplang/IImplementation.h"
#include "maplang/ImplementationFactory.h"
#include "maplang/Instance.h"

namespace maplang {

class Graph final {
 public:
  GraphEdge& connect(
      const std::string& fromNodeName,
      const std::string& fromChannel,
      const std::string& toNodeName);

  void disconnect(
      const std::string& fromNodeName,
      const std::string& fromChannel,
      const std::string& toNodeName);

  using NodeVisitor =
      std::function<void(const std::shared_ptr<GraphNode>& graphNode)>;

  /**
   * Visits GraphNodes. Order is unspecified.
   * @param visitor Called for each GraphNode in the Graph.
   */
  void visitNodes(const NodeVisitor& visitor) const;

  void visitNodesHeadsLast(const NodeVisitor& visitor) const;

  bool hasNode(const std::string& nodeName) const;

  std::shared_ptr<GraphNode> createGraphNode(
      const std::string& nodeName,
      bool allowIncomingConnections,
      bool allowOutgoingConnections);

  std::optional<std::shared_ptr<GraphNode>> getNode(
      const std::string& nodeName) const;

  std::shared_ptr<GraphNode> getNodeOrThrow(
      const std::string& nodeName) const;

 private:
  std::unordered_map<std::string, std::shared_ptr<GraphNode>> mNameToNodeMap;

  std::shared_ptr<ImplementationFactory> mNodeFactory;
};

}  // namespace maplang

#endif  // __MAPLANG_GRAPH_H_
