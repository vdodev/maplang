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

#include "../../IPartitionedNode.h"
#include <unordered_map>

namespace dgraph {

class UvTcpServerImpl;

class UvTcpServerNode : public IPartitionedNode {
 public:
  UvTcpServerNode();
  ~UvTcpServerNode() override = default;

  size_t getPartitionCount() override;
  std::string getPartitionName(size_t partitionIndex) override;

  std::shared_ptr<INode> getPartition(const std::string& partition) override;

 private:
  const std::shared_ptr<UvTcpServerImpl> mImpl;
  std::unordered_map<std::string, std::shared_ptr<INode>> mNodes;
};

}  // namespace dgraph
