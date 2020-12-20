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

#ifndef MAPLANG_INCLUDE_PRIVATE_NODES_PASSTHROUGHNODE_H_
#define MAPLANG_INCLUDE_PRIVATE_NODES_PASSTHROUGHNODE_H_

#include "maplang/INode.h"

namespace maplang {

class PassThroughNode : public INode, public IPathable {
 public:
  static const std::string kInputParam_OutputChannel;

 public:
  PassThroughNode(const nlohmann::json& initParameters);
  ~PassThroughNode() override = default;

  void handlePacket(const PathablePacket& incomingPacket) override;

  IPathable* asPathable() override { return this; }
  ISink* asSink() override { return nullptr; }
  ISource* asSource() override { return nullptr; }
  ICohesiveGroup* asGroup() override { return nullptr; }

 private:
  const std::string mOutputChannel;
};

}  // namespace maplang

#endif  // MAPLANG_INCLUDE_PRIVATE_NODES_PASSTHROUGHNODE_H_
