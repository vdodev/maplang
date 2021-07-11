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
#include <unordered_map>

#include "maplang/Graph.h"
#include "maplang/ICohesiveGroup.h"
#include "maplang/INode.h"
#include "maplang/IUvLoopRunnerFactory.h"
#include "maplang/NodeFactory.h"

namespace maplang {

class ThreadGroup;
class DataGraphImpl;

void to_json(nlohmann::json& j, const PacketDeliveryType& packetDelivery);
void from_json(const nlohmann::json& j, PacketDeliveryType& packetDelivery);

class DataGraph final {
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
      const std::string& fromNodeName,
      const std::string& fromChannel,
      const std::string& toNodeName,
      const PacketDeliveryType& sameThreadQueueToTargetType =
          PacketDeliveryType::PushDirectlyToTarget);

  void disconnect(
      const std::string& fromNodeName,
      const std::string& fromChannel,
      const std::string& toNodeName);

  void sendPacket(const Packet& packet, const std::string& toNodeName);

  void setThreadGroupForInstance(
      const std::string& instanceName,
      const std::string& threadGroup);

  void validateConnections() const;

  /**
   * An instance is an instantiated INode. It doesn't have a concept of
   * connections.
   *
   * GraphElements reference an instance, which allows a single
   * instantiation to be used in multiple paths.
   */
  void setNodeInstance(
      const std::string& nodeName,
      const std::string& instanceName);

  void setInstanceInitParameters(
      const std::string& instanceName,
      const nlohmann::json& initParameters);

  /**
   * Sets the type to be instantiated from the NodeFactory.
   */
  void setInstanceType(
      const std::string& instanceName,
      const std::string& typeName);

  /**
   * Assigns the implementation to the instance.
   *
   * If another implementation was already instantiated, it will be removed.
   */
  void setInstanceImplementation(
      const std::string& instanceName,
      const std::shared_ptr<INode>& implementation);

  void setInstanceImplementationToGroupInterface(
      const std::string& instanceName,
      const std::string& groupInstanceName,
      const std::string& groupInterfaceName);

  void setNodeFactory(const std::shared_ptr<NodeFactory>& factory);

  void visitGraphElements(const Graph::GraphElementVisitor& visitor) const;

 private:
  const std::shared_ptr<DataGraphImpl> impl;
};

}  // namespace maplang

#endif  // __MAPLANG_DATA_GRAPH_H__
