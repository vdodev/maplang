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

#ifndef __MAPLANG_UVTCPCONNECTIONGROUP_H__
#define __MAPLANG_UVTCPCONNECTIONGROUP_H__

#include <unordered_map>

#include "maplang/ICohesiveGroup.h"

namespace maplang {

class UvTcpImpl;

class UvTcpConnectionGroup : public ICohesiveGroup, public INode, public ISource {
 public:
  UvTcpConnectionGroup();
  ~UvTcpConnectionGroup() override = default;

  size_t getNodeCount() override;
  std::string getNodeName(size_t nodeIndex) override;

  std::shared_ptr<INode> getNode(const std::string& nodeName) override;

  IPathable* asPathable() override { return nullptr; }
  ISink* asSink() override { return nullptr; }
  ISource* asSource() override { return this; }
  ICohesiveGroup* asGroup() override { return this; }

  void setPacketPusher(
      const std::shared_ptr<IPacketPusher>& pusher) override;

 private:
  const std::shared_ptr<UvTcpImpl> mImpl;
  std::unordered_map<std::string, std::shared_ptr<INode>> mNodes;
};

}  // namespace maplang

#endif  // __MAPLANG_UVTCPCONNECTIONGROUP_H__
