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

#ifndef __DGRAPH_CONTEXTUAL_NODE_H__
#define __DGRAPH_CONTEXTUAL_NODE_H__

#include "IPartitionedNode.h"

namespace dgraph {
class ContextRouter;

class ContextualNode final : public IPartitionedNode {
 public:
  ContextualNode(const std::string& nodeName, const std::string& key,
                 const nlohmann::json& initData);
  ~ContextualNode() override = default;

  size_t getPartitionCount() override;
  virtual std::string getPartitionName(size_t partitionIndex) override;

  virtual std::shared_ptr<INode> getPartition(
      const std::string& partition) override;

 private:
  const std::shared_ptr<ContextRouter> mContextRouter;
  const std::shared_ptr<INode> mContextRemover;
  std::unordered_map<std::string, std::shared_ptr<INode>> mNodeMap;
};

}  // namespace dgraph

#endif  // __DGRAPH_CONTEXTUAL_NODE_H__

/*
 * How is instance-deletion handled?
 * An upstream node might send a message along some channel which means it
 * should be removed. Something down-stream may want to terminate a connection
 * (http request too large). In the TCP/HTTP case, the TCP module can send a
 * packet from its "disconnected" channel. If it is disconnected by the client,
 * other module sending packet to disconnect, etc., it will tear down its
 * downstream instances.
 *
 * ContextualSink is partitioned.
 * New instances are created in handlePacket() when a new key is detected.
 * Instances are removed by sending a packet to the "remove" partition, and
 * containing the same key/value pair used for handlePacket() lookups.
 *
 * Should this be a ContextualNode instead of sink?
 * The sink is the only part that is contextual.
 * If its also a source, it will get the same packet pusher the main class got.
 *
 * Could it produce a pathable?
 * If the factory produces a pathable, our router node should be a pathable too.
 * If the factory produces a sink, the router node is a sync.
 * If the factory produces a source, the router node is also a source so we can
 *   get the packet pusher.
 */
