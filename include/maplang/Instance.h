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

#ifndef MAPLANG_INCLUDE_MAPLANG_INSTANCE_H_
#define MAPLANG_INCLUDE_MAPLANG_INSTANCE_H_

#include "maplang/INode.h"

namespace maplang {

class NodeFactory;
class ThreadGroup;

class Instance final {
 public:
  void setType(
      const std::string& typeName,
      const std::shared_ptr<NodeFactory>& nodeFactory);
  std::string getType() const { return mTypeName; }

  void setImplementation(const std::shared_ptr<INode>& implementation);
  std::shared_ptr<INode> getImplementation() const { return mImplementation; }

  void setInitParameters(const nlohmann::json& initParameters);
  nlohmann::json getInitParameters() const { return mInitParameters; }

  void setSubgraphContext(
      const std::shared_ptr<ISubgraphContext>& subgraphContext);
  std::shared_ptr<ISubgraphContext> getSubgraphContext() const;

  void setThreadGroupName(const std::string& threadGroupName);
  std::string getThreadGroupName() const;

  void setPacketPusherForISources(
      const std::shared_ptr<IPacketPusher>& packetPusher);
  std::shared_ptr<IPacketPusher> getPacketPusherForISources() const;

 private:
  std::string mTypeName;
  std::shared_ptr<INode> mImplementation;
  nlohmann::json mInitParameters;
  std::shared_ptr<ISubgraphContext> mSubgraphContext;
  std::string mThreadGroupName;
  std::weak_ptr<IPacketPusher> mPacketPusherForISources;
};

}  // namespace maplang

#endif  // MAPLANG_INCLUDE_MAPLANG_INSTANCE_H_
