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

#ifndef DATA_GRAPH_GRAPH_GRAPH_H_
#define DATA_GRAPH_GRAPH_GRAPH_H_

#include <list>
#include <functional>

#include "Edge.h"
#include "GraphElement.h"
#include "../INode.h"

namespace dgraph {

class Graph final {
 public:
  void connect(
      const std::shared_ptr<INode>& fromNode,
      const std::string& fromChannel,
      const std::shared_ptr<INode>& toNode,
      const std::string& fromPathableId = "",
      const std::string& toPathableId = "");

  void disconnect(
      const std::shared_ptr<INode>& fromNode,
      const std::string& fromChannel,
      const std::shared_ptr<INode>& toNode,
      const std::string& fromPathableId = "",
      const std::string& toPathableId = "");

  using GraphElementVisitor = std::function<void (const std::string& pathableId, const std::shared_ptr<GraphElement>& graphElement)>;

  void visitGraphElements(const GraphElementVisitor& visitor) const;

  bool hasNode(const std::shared_ptr<INode>& node, const std::string& pathableId) const;

  std::shared_ptr<GraphElement> getOrCreateGraphElement(const std::shared_ptr<INode>& node, const std::string& pathableId);

 private:
  using GraphElementLookupKey = std::pair<const INode*, std::string>;

  struct GraphElementKeyHasher {
    std::size_t operator()(const GraphElementLookupKey& key) const {
      return std::hash<decltype(key.first)>()(key.first) ^
          std::hash<decltype(key.second)>()(key.second);
    }
  };

  std::unordered_map<GraphElementLookupKey, std::shared_ptr<GraphElement>, GraphElementKeyHasher> mNodeToGraphElementMap;

 private:
  void validateNodeTypesAreCompatible(const std::shared_ptr<INode>& node) const;
};

}  // namespace dgraph
#endif //DATA_GRAPH_GRAPH_GRAPH_H_
