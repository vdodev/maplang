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

#ifndef MAPLANG_INCLUDE_MAPLANG_DATAGRAPHNODE_H_
#define MAPLANG_INCLUDE_MAPLANG_DATAGRAPHNODE_H_

#include "maplang/INode.h"
#include "maplang/ICohesiveGroup.h"

namespace maplang {

class DataGraphNode final : public INode, public ICohesiveGroup {
 public:
  static const std::string kNode_Connect;
  static const std::string kNode_Disconnect;
  static const std::string kNode_CreateNode;
  static const std::string kNode_SendPacketToNode;

  static const std::string kInputParam_FromNode;
  static const std::string kInputParam_FromPathableId;
  static const std::string kInputParam_ToNode;
  static const std::string kInputParam_ToPathableId;
  static const std::string kInputParam_Channel;
  static const std::string kInputParam_PacketDeliveryType;
  static const std::string kInputParam_InitParameters;
  static const std::string kInputParam_PacketParameters;
  static const std::string kInputParam_NodesToCreate;
  static const std::string kInputParam_NodeImplementation;

 public:
  DataGraphNode();
  ~DataGraphNode() override = default;

  size_t getNodeCount() override;
  virtual std::string getNodeName(size_t partitionIndex) override;

  virtual std::shared_ptr<INode> getNode(const std::string& partition) override;

  IPathable* asPathable() override { return nullptr; }
  ISink* asSink() override { return nullptr; }
  ISource* asSource() override { return nullptr; }
  ICohesiveGroup* asGroup() override { return this; }

 public:
  struct Impl;

 private:
  const std::shared_ptr<Impl> mImpl;
  std::unordered_map<std::string, std::shared_ptr<INode>> mNodes;
};

}  // namespace maplang
#endif  // MAPLANG_INCLUDE_MAPLANG_DATAGRAPHNODE_H_
