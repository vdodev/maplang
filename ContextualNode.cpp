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

#include "maplang/ContextualNode.h"

#include "maplang/NodeRegistration.h"
#include "logging.h"
#include <sstream>

using namespace std;
using namespace nlohmann;

namespace maplang {

static const string kPartitionName_ContextRouter = "Context Router";
static const string kPartitionName_ContextRemover = "Context Remover";

class ContextRouter : public INode,
                      public ISink,
                      public ISource,
                      public IPathable {
 public:
  static bool nodeIsSink(const string& nodeName, const json& initParams) {
    const auto node = NodeRegistration::defaultRegistration()->createNode(
        nodeName, initParams);
    return node != nullptr && node->asSink() != nullptr;
  }

  static bool nodeIsSource(const string& nodeName, const json& initParams) {
    const auto node = NodeRegistration::defaultRegistration()->createNode(
        nodeName, initParams);
    return node != nullptr && node->asSource() != nullptr;
  }

  static bool nodeIsPathable(const string& nodeName, const json& initParams) {
    const auto node = NodeRegistration::defaultRegistration()->createNode(
        nodeName, initParams);
    return node != nullptr && node->asPathable() != nullptr;
  }

  ContextRouter(const string& nodeName, const string& key,
                const json& initParameters)
      : mThisAsSink(nodeIsSink(nodeName, initParameters) ? this : nullptr),
        mThisAsSource(nodeIsSource(nodeName, initParameters) ? this : nullptr),
        mThisAsPathable(nodeIsPathable(nodeName, initParameters) ? this
                                                                 : nullptr),
        mNodeName(nodeName),
        mKey(key),
        mInitParameters(initParameters) {}

  ~ContextRouter() override = default;

  void handlePacket(const PathablePacket* packet) override {
    const auto contextLookup = packet->parameters[mKey].get<string>();
    shared_ptr<INode> node;
    const auto it = mNodes.find(contextLookup);
    if (it != mNodes.end()) {
      node = it->second;
    } else {
      mNodes[contextLookup] =
          NodeRegistration::defaultRegistration()->createNode(mNodeName,
                                                              mInitParameters);
      node = mNodes[contextLookup];
    }

    node->asPathable()->handlePacket(packet);
  }

  void handlePacket(const Packet* packet) override {
    if (!packet->parameters.contains(mKey)) {
      throw runtime_error("Packet parameters must contain key '" + mKey + "'.");
    }
    const auto contextLookup = packet->parameters[mKey].get<string>();
    shared_ptr<INode> node;
    const auto it = mNodes.find(contextLookup);
    if (it != mNodes.end()) {
      node = it->second;
    } else {
      mNodes[contextLookup] =
          NodeRegistration::defaultRegistration()->createNode(mNodeName,
                                                              mInitParameters);
      node = mNodes[contextLookup];
      node->setSubgraphContext(mSubgraphContext);

      auto source = node->asSource();
      if (source) {
        source->setPacketPusher(mPacketPusher);
      }
    }

    node->asSink()->handlePacket(packet);
  }

  void setSubgraphContext(
      const shared_ptr<ISubgraphContext>& context) override {
    mSubgraphContext = context;

    for (auto& pair : mNodes) {
      pair.second->setSubgraphContext(context);
    }
  }

  bool removeNode(const string& nodeKey) {
    auto it = mNodes.find(nodeKey);
    if (it == mNodes.end()) {
      return false;
    }

    mNodes.erase(it);
    return true;
  }

  IPathable* asPathable() override { return mThisAsPathable; }

  ISink* asSink() override { return mThisAsSink; }

  ISource* asSource() override { return mThisAsSource; }

  void setPacketPusher(const shared_ptr<IPacketPusher>& pusher) override;

 private:
  const string mNodeName;
  const string mKey;
  const json mInitParameters;

  IPathable* const mThisAsPathable;
  ISource* const mThisAsSource;
  ISink* const mThisAsSink;

  shared_ptr<ISubgraphContext> mSubgraphContext;
  shared_ptr<IPacketPusher> mPacketPusher;

  unordered_map<string, shared_ptr<INode>> mNodes;
};

class ContextualPacketPusher : public IPacketPusher {
 public:
  ContextualPacketPusher(const shared_ptr<IPacketPusher>& wrapped,
                         ContextRouter* contextRouter, const string& nodeKey)
      : mWrappedPusher(wrapped),
        mContextRouter(contextRouter),
        mNodeKey(nodeKey) {}

  void pushPacket(const Packet* packet, const string& channelName) override {
    mWrappedPusher->pushPacket(packet, channelName);
  }

 private:
  const shared_ptr<IPacketPusher> mWrappedPusher;
  ContextRouter* const mContextRouter;
  const string mNodeKey;
};

void ContextRouter::setPacketPusher(const shared_ptr<IPacketPusher>& pusher) {
  mPacketPusher = make_shared<ContextualPacketPusher>(pusher, this, mKey);

  for (auto& pair : mNodes) {
    auto source = pair.second->asSource();

    if (source) {
      source->setPacketPusher(pusher);
    }
  }
}

class ContextRemover : public INode, public IPathable {
 public:
  ContextRemover(const weak_ptr<ContextRouter>& contextRouter,
                 const string& key)
      : mContextRouter(contextRouter), mKey(key) {}

  void handlePacket(const PathablePacket* incomingPacket) override {
    const auto contextRouter = mContextRouter.lock();
    if (!contextRouter) {
      return;
    }

    string nodeKey = incomingPacket->parameters[mKey].get<string>();
    if (!contextRouter->removeNode(nodeKey)) {
      return;
    }

    Packet removedHandleKeyPacket;
    removedHandleKeyPacket.parameters[mKey] = incomingPacket->parameters[mKey];
    incomingPacket->packetPusher->pushPacket(&removedHandleKeyPacket,
                                             "Removed Key");
  }

  IPathable* asPathable() override { return this; }
  ISink* asSink() override { return nullptr; }
  ISource* asSource() override { return nullptr; }

 private:
  const weak_ptr<ContextRouter> mContextRouter;
  const string mKey;
};

ContextualNode::ContextualNode(const string& nodeName, const string& key,
                               const json& initData)
    : mContextRouter(make_shared<ContextRouter>(nodeName, key, initData)),
      mContextRemover(make_shared<ContextRemover>(mContextRouter, key)) {
  mNodeMap[kPartitionName_ContextRouter] = mContextRouter;
  mNodeMap[kPartitionName_ContextRemover] = mContextRemover;
}

size_t ContextualNode::getNodeCount() { return mNodeMap.size(); }

string ContextualNode::getNodeName(size_t partitionIndex) {
  switch (partitionIndex) {
    case 0:
      return kPartitionName_ContextRouter;
    case 1:
      return kPartitionName_ContextRemover;
    default:
      return nullptr;
  }
}

shared_ptr<INode> ContextualNode::getNode(const string& nodeName) {
  auto it = mNodeMap.find(nodeName);
  if (it == mNodeMap.end()) {
    return nullptr;
  }

  return it->second;
}

}  // namespace maplang
