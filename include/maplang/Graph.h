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

#ifndef __MAPLANG_GRAPH_H_
#define __MAPLANG_GRAPH_H_

#include <functional>
#include <list>

#include "maplang/INode.h"
#include "maplang/Instance.h"
#include "maplang/NodeFactory.h"

namespace maplang {

enum class PacketDeliveryType { PushDirectlyToTarget, AlwaysQueue };

struct GraphElement;

struct GraphEdge final {
  std::string channel;
  std::shared_ptr<GraphElement> next;

  PacketDeliveryType sameThreadQueueToTargetType =
      PacketDeliveryType::PushDirectlyToTarget;
};

struct ThreadGroup;

struct GraphElement final {
 public:
  GraphElement(const std::string& elementName);
  void cleanUpEmptyEdges();

 public:
  const std::string elementName;
  std::string instanceName;
  std::shared_ptr<IPacketPusher> packetPusher;

  // for parameter propagation when the downstream node(s) get a packet from
  // this node.
  std::shared_ptr<const nlohmann::json> lastReceivedParameters;
  std::list<std::weak_ptr<GraphElement>> backEdges;

  // All GraphElements this one connects to. channel => edges from this channel
  std::unordered_map<std::string, std::vector<GraphEdge>> forwardEdges;
};

class Graph final {
 public:
  GraphEdge& connect(
      const std::string& fromElementName,
      const std::string& fromChannel,
      const std::string& toItem);

  void disconnect(
      const std::string& fromItem,
      const std::string& fromChannel,
      const std::string& toItem);

  using GraphElementVisitor =
      std::function<void(const std::shared_ptr<GraphElement>& graphElement)>;

  /**
   * Visits GraphElements. Order is unspecified.
   * @param visitor Called for each GraphElement in the Graph.
   */
  void visitGraphElements(const GraphElementVisitor& visitor) const;

  void visitGraphElementsHeadsLast(const GraphElementVisitor& visitor) const;

  bool hasElement(const std::string& elementName) const;

  std::shared_ptr<GraphElement> getOrCreateGraphElement(
      const std::string& elementName);

  std::optional<std::shared_ptr<GraphElement>> getGraphElement(
      const std::string& elementName) const;

 private:
  std::unordered_map<std::string, std::shared_ptr<GraphElement>>
      mNameToElementMap;

  std::shared_ptr<NodeFactory> mNodeFactory;
};

}  // namespace maplang

#endif  // __MAPLANG_GRAPH_H_
