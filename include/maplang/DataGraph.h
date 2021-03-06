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

#ifndef __MAPLANG_DATA_GRAPH_H__
#define __MAPLANG_DATA_GRAPH_H__

#include <memory>

#include "maplang/INode.h"
#include "maplang/ICohesiveGroup.h"
#include "maplang/IUvLoopRunnerFactory.h"

namespace maplang {

class DataGraphImpl;

enum class PacketDeliveryType { PushDirectlyToTarget, AlwaysQueue };

void to_json(nlohmann::json& j, const PacketDeliveryType& packetDelivery);
void from_json(const nlohmann::json& j, PacketDeliveryType& packetDelivery);

class DataGraph final{
 public:
  static const std::string kDefaultThreadGroupName;

 public:
  DataGraph();
  ~DataGraph();

  /*
   * Need unique Node IDs so Packets can connect nodes.
   * connect(), disconnect(), possibly other methods should be subnodes.
   */
  void connect(
      const std::shared_ptr<INode>& fromNode,
      const std::string& fromChannel,
      const std::shared_ptr<INode>& toNode,
      const std::string& fromPathableId = "",
      const std::string& toPathableId = "",
      const PacketDeliveryType& sameThreadQueueToTargetType =
          PacketDeliveryType::PushDirectlyToTarget);

  void disconnect(
      const std::shared_ptr<INode>& fromNode,
      const std::string& fromChannel,
      const std::shared_ptr<INode>& toNode,
      const std::string& fromPathableId = "",
      const std::string& toPathableId = "");

  void sendPacket(
      const Packet& packet,
      const std::shared_ptr<INode>& toNode,
      const std::string& toPathableId = "");

  void sendPacket(
      Packet&& packet,
      const std::shared_ptr<INode>& toNode,
      const std::string& toPathableId = "");

  void setThreadGroupForNode(
      const std::shared_ptr<INode>& toNode,
      const std::string& threadGroup,
      const std::string& pathableId = "");

 private:
  const std::shared_ptr<DataGraphImpl> impl;
};

}  // namespace maplang

#endif  // __MAPLANG_DATA_GRAPH_H__
