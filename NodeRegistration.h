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

#pragma once

#include <memory>
#include <set>

#include "INode.h"
#include "ICohesiveNode.h"
#include "json.hpp"

namespace dgraph {

class NodeRegistration {
 public:
  using NodeFactory = std::function<std::shared_ptr<INode>(
      const nlohmann::json& initParameters)>;
  using PartitionedNodeFactory =
      std::function<std::shared_ptr<ICohesiveNode>(
          const nlohmann::json& initParameters)>;

  using NodeNameVisitor = std::function<void(const std::string& nodeName)>;

  static NodeRegistration* defaultRegistration();

  void registerNodeFactory(const std::string& name, NodeFactory&& factory);
  void registerPartitionedNodeFactory(const std::string& name,
                                      PartitionedNodeFactory&& factory);

  std::shared_ptr<INode> createNode(const std::string& name,
                                    const nlohmann::json& initParameters);
  std::shared_ptr<ICohesiveNode> createPartitionedNode(
      const std::string& name, const nlohmann::json& initParameters);

  void visitNodeNames(const NodeNameVisitor& visitor);
  void visitPartitionedNodeNames(const NodeNameVisitor& visitor);

 private:
  std::unordered_map<std::string, NodeFactory> mNodeFactoryMap;
  std::unordered_map<std::string, PartitionedNodeFactory>
      mPartitionedNodeFactoryMap;
};

}  // namespace dgraph
