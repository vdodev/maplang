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

#ifndef MAPLANG_DATA_GRAPH_H_
#define MAPLANG_DATA_GRAPH_H_

#include <memory>
#include <unordered_map>

#include "maplang/Factories.h"
#include "maplang/Graph.h"
#include "maplang/IGroup.h"
#include "maplang/IImplementation.h"

namespace maplang {

class ThreadGroup;
class DataGraphImpl;

class DataGraph final : public IGroup {
 public:
  static const std::string kDefaultThreadGroupName;

 public:
  explicit DataGraph(const Factories& factories);
  ~DataGraph() override = default;

  std::shared_ptr<GraphNode>
  createNode(const std::string& name, bool allowIncoming, bool allowOutgoing);

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

  /**
   * An instance is an instantiated IImplementation. It doesn't have a concept
   * of connections.
   *
   * GraphNodes reference an instance, which allows a single
   * instantiation to be used in multiple paths.
   */
  void setNodeInstance(
      const std::string& nodeName,
      const std::string& instanceName);

  void setInstanceInitParameters(
      const std::string& instanceName,
      const nlohmann::json& initParameters);

  /**
   * Adds parameters, but does not overwrite existing parameters.
   */
  void insertInstanceInitParameters(
      const std::string& instanceName,
      const nlohmann::json& initParameters);

  /**
   * Sets the type to be instantiated from the ImplementationFactory.
   */
  void setInstanceType(
      const std::string& instanceName,
      const std::string& typeName);

  void setInstanceImplementation(
      const std::string& instanceName,
      const std::shared_ptr<IImplementation>& implementation);

  void setInstanceImplementationToGroupInterface(
      const std::string& instanceName,
      const std::string& groupInstanceName,
      const std::string& groupInterfaceName);

  void visitNodes(const Graph::NodeVisitor& visitor) const;

  size_t getInterfaceCount() override;
  std::string getInterfaceName(size_t nodeIndex) override;
  std::shared_ptr<IImplementation> getInterface(
      const std::string& nodeName) override;

  void startGraph();

 private:
  const std::shared_ptr<DataGraphImpl> impl;
};

}  // namespace maplang

#endif  // MAPLANG_DATA_GRAPH_H_
