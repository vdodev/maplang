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

#include "nodes/ContextualNode.h"

#include <sstream>

#include "logging.h"
#include "maplang/NodeRegistration.h"

using namespace std;
using namespace nlohmann;

namespace maplang {

static const string kPartitionName_ContextRouter = "Context Router";
static const string kPartitionName_ContextRemover = "Context Remover";

static const string kInitDataParameter_Key = "key";
static const string kInitDataParameter_NodeImplementation =
    "nodeImplementation";

class IContextRouter {
 public:
  virtual ~IContextRouter() = default;

  virtual shared_ptr<INode> asINode() = 0;
  virtual void addNode(
      const string& contextLookup,
      const shared_ptr<INode>& node) = 0;
  virtual bool removeNode(const string& contextLookup) = 0;
};

class IRouterInstanceCreator {
 public:
  virtual ~IRouterInstanceCreator() = default;

  /**
   * Creates a new INode, and calls IContextRouter::addNode() on all subnodes
   * (one for a SingleNodeRouter, and 1+ for a CohesiveGroupRouter).
   */
  virtual void createNewInstance(const string& forNewContextLookup) = 0;
};

class SingleNodeRouter;

class ContextualNode::Impl
    : public IRouterInstanceCreator,
      public enable_shared_from_this<ContextualNode::Impl> {
 public:
  Impl(const json& initData);
  ~Impl() override;

  void initialize();
  void createNewInstance(const string& forNewContextLookup) override;

 public:
  const json mInitParameters;
  const string mNodeImplementation;
  const string mKey;

  shared_ptr<IContextRouter> mContextRouter;
  shared_ptr<INode> mContextRemover;
  unordered_map<string, shared_ptr<INode>> mNodeMap;
};

class CohesiveGroupRouter
    : public INode,
      public ICohesiveGroup,
      public IRouterInstanceCreator,
      public IContextRouter,
      public enable_shared_from_this<CohesiveGroupRouter> {
 public:
  CohesiveGroupRouter(
      const weak_ptr<IRouterInstanceCreator>& subinstanceCreator,
      ICohesiveGroup* const templateGroup,
      const string& key);

  // Implementation needs to pass a weak pointer of "this", which can only be
  // done after the constructor.
  void initialize(ICohesiveGroup* const templateGroup);

  ~CohesiveGroupRouter() override = default;

  static vector<string> createGroupNamesVector(
      ICohesiveGroup* const templateGroup);
  unordered_map<string, shared_ptr<IContextRouter>> createGroupRouterNodes(
      const weak_ptr<IRouterInstanceCreator>& subinstanceCreator,
      ICohesiveGroup* const templateGroup,
      const string& key);

  shared_ptr<INode> asINode() { return shared_from_this(); }
  void addNode(const string& contextLookup, const shared_ptr<INode>& node)
      override;
  bool removeNode(const string& contextLookup) override;
  void createNewInstance(const string& forNewContextLookup) override;

  IPathable* asPathable() override { return nullptr; }
  ISink* asSink() override { return nullptr; }
  ISource* asSource() override { return nullptr; }
  ICohesiveGroup* asGroup() override { return this; }

  size_t getNodeCount() override;
  string getNodeName(size_t nodeIndex) override;

  shared_ptr<INode> getNode(const string& nodeName) override;

 private:
  const weak_ptr<IRouterInstanceCreator> mSubinstanceCreator;
  const json mNewCohesiveGroupInitParameters;
  const size_t mGroupNodeCount;
  const vector<string> mGroupNodeNames;
  const string mKey;
  unordered_map<string, shared_ptr<IContextRouter>> mNodeRouters;
};

class SingleNodeRouter : public INode,
                         public ISink,
                         public ISource,
                         public IPathable,
                         public ISubgraphContext,
                         public IContextRouter,
                         public enable_shared_from_this<SingleNodeRouter> {
 public:
  SingleNodeRouter(
      const weak_ptr<IRouterInstanceCreator>& instanceCreator,
      const shared_ptr<INode>& templateNode,
      const string& key)
      : mInstanceCreator(instanceCreator),
        mThisAsSink(templateNode->asSink() ? this : nullptr),
        mThisAsSource(templateNode->asSource() ? this : nullptr),
        mThisAsPathable(templateNode->asPathable() ? this : nullptr),
        mKey(key) {}

  ~SingleNodeRouter() override = default;

  shared_ptr<INode> asINode() { return shared_from_this(); }

  void addNode(const string& contextLookup, const shared_ptr<INode>& node)
      override {
    mNodes[contextLookup] = node;
    mReverseNodeLookup[node.get()] = contextLookup;

    node->setSubgraphContext(shared_from_this());

    auto source = node->asSource();
    if (source) {
      source->setPacketPusher(mPacketPusher);
    }
  }

  void handlePacket(const PathablePacket& incomingPathablePacket) override {
    const Packet& incomingPacket = incomingPathablePacket.packet;
    const auto contextLookup = incomingPacket.parameters[mKey].get<string>();
    shared_ptr<INode> node;
    auto it = mNodes.find(contextLookup);
    if (it != mNodes.end()) {
      node = it->second;
    } else {
      const auto instanceCreator = mInstanceCreator.lock();
      if (instanceCreator == nullptr) {
        loge("Instance creator went away.\n");
        return;
      }

      // calls our addInstance(). For groups, calls all group subnode's
      // addInstance().
      instanceCreator->createNewInstance(contextLookup);
      node = mNodes[contextLookup];
    }

    node->asPathable()->handlePacket(incomingPathablePacket);
  }

  void handlePacket(const Packet& packet) override {
    if (!packet.parameters.contains(mKey)) {
      throw runtime_error("Packet parameters must contain key '" + mKey + "'.");
    }
    const auto contextLookup = packet.parameters[mKey].get<string>();
    shared_ptr<INode> node;
    const auto it = mNodes.find(contextLookup);
    if (it != mNodes.end()) {
      node = it->second;
    } else {
      const auto instanceCreator = mInstanceCreator.lock();
      instanceCreator->createNewInstance(contextLookup);

      node = mNodes[contextLookup];
    }

    node->asSink()->handlePacket(packet);
  }

  void setSubgraphContext(
      const shared_ptr<ISubgraphContext>& context) override {
    mOriginalSubgraphContext = context;
  }

  bool removeNode(const string& contextLookup) {
    auto nodeIt = mNodes.find(contextLookup);
    if (nodeIt == mNodes.end()) {
      return false;
    }

    const auto node = nodeIt->second;
    auto reverseLookupIt = mReverseNodeLookup.find(node.get());

    mNodes.erase(nodeIt);
    mReverseNodeLookup.erase(reverseLookupIt);

    return true;
  }

  shared_ptr<uv_loop_t> getUvLoop() const override {
    return mOriginalSubgraphContext->getUvLoop();
  }

  void removeFromGraph(INode* node) override {
    auto it = mReverseNodeLookup.find(node);
    if (it == mReverseNodeLookup.end()) {
      return;
    }

    const string& key = it->second;
    mNodes.erase(key);
    mReverseNodeLookup.erase(it);
  }

  IPathable* asPathable() override { return mThisAsPathable; }
  ISink* asSink() override { return mThisAsSink; }
  ISource* asSource() override { return mThisAsSource; }
  ICohesiveGroup* asGroup() override { return nullptr; }

  void setPacketPusher(const shared_ptr<IPacketPusher>& pusher) override;

 private:
  const weak_ptr<IRouterInstanceCreator> mInstanceCreator;
  IPathable* const mThisAsPathable;
  ISource* const mThisAsSource;
  ISink* const mThisAsSink;
  const string mKey;

  shared_ptr<ISubgraphContext> mOriginalSubgraphContext;
  shared_ptr<IPacketPusher> mPacketPusher;

  unordered_map<string, shared_ptr<INode>> mNodes;
  unordered_map<INode*, string> mReverseNodeLookup;
};

static shared_ptr<IContextRouter> createContextRouter(
    const weak_ptr<IRouterInstanceCreator>& subinstanceCreator,
    const shared_ptr<INode>& templateNode,
    const string& key) {
  if (templateNode->asGroup() != nullptr) {
    auto templateGroup = templateNode->asGroup();
    auto router = make_shared<CohesiveGroupRouter>(
        subinstanceCreator,
        templateGroup,
        key);
    router->initialize(templateGroup);

    return router;
  } else {
    return make_shared<SingleNodeRouter>(subinstanceCreator, templateNode, key);
  }
}

class ContextualPacketPusher : public IPacketPusher {
 public:
  ContextualPacketPusher(
      const shared_ptr<IPacketPusher>& wrapped,
      SingleNodeRouter* contextRouter,
      const string& nodeKey)
      : mWrappedPusher(wrapped), mContextRouter(contextRouter),
        mNodeKey(nodeKey) {}

  void pushPacket(const Packet& packet, const string& channelName) override {
    mWrappedPusher->pushPacket(packet, channelName);
  }

  void pushPacket(Packet&& packet, const string& channelName) override {
    mWrappedPusher->pushPacket(move(packet), channelName);
  }

 private:
  const shared_ptr<IPacketPusher> mWrappedPusher;
  SingleNodeRouter* const mContextRouter;
  const string mNodeKey;
};

void SingleNodeRouter::setPacketPusher(
    const shared_ptr<IPacketPusher>& pusher) {
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
  ContextRemover(
      const weak_ptr<IContextRouter>& contextRouter,
      const string& key)
      : mContextRouter(contextRouter), mKey(key) {}

  void handlePacket(const PathablePacket& incomingPathablePacket) override {
    const Packet& incomingPacket = incomingPathablePacket.packet;
    const auto contextRouter = mContextRouter.lock();
    if (!contextRouter) {
      return;
    }

    string nodeKey = incomingPacket.parameters[mKey].get<string>();
    if (!contextRouter->removeNode(nodeKey)) {
      return;
    }

    Packet removedHandleKeyPacket;
    removedHandleKeyPacket.parameters[mKey] = incomingPacket.parameters[mKey];
    incomingPathablePacket.packetPusher->pushPacket(
        move(removedHandleKeyPacket),
        "Removed Key");
  }

  IPathable* asPathable() override { return this; }
  ISink* asSink() override { return nullptr; }
  ISource* asSource() override { return nullptr; }
  ICohesiveGroup* asGroup() override { return nullptr; }

 private:
  const weak_ptr<IContextRouter> mContextRouter;
  const string mKey;
};

CohesiveGroupRouter::CohesiveGroupRouter(
    const weak_ptr<IRouterInstanceCreator>& subinstanceCreator,
    ICohesiveGroup* const templateGroup,
    const string& key)
    : mSubinstanceCreator(subinstanceCreator),
      mGroupNodeCount(templateGroup->getNodeCount()),
      mGroupNodeNames(createGroupNamesVector(templateGroup)), mKey(key) {}

void CohesiveGroupRouter::initialize(ICohesiveGroup* const templateGroup) {
  mNodeRouters =
      createGroupRouterNodes(shared_from_this(), templateGroup, mKey);
}

vector<string> CohesiveGroupRouter::createGroupNamesVector(
    ICohesiveGroup* const templateGroup) {
  const size_t nodeCount =
      templateGroup != nullptr ? templateGroup->getNodeCount() : 0;
  vector<string> nodeNames(nodeCount);
  if (templateGroup == nullptr) {
    return nodeNames;
  }

  for (size_t i = 0; i < nodeCount; i++) {
    const string groupNodeName = templateGroup->getNodeName(i);
    nodeNames[i] = groupNodeName;
  }

  return nodeNames;
}

unordered_map<string, shared_ptr<IContextRouter>>
CohesiveGroupRouter::createGroupRouterNodes(
    const weak_ptr<IRouterInstanceCreator>& subinstanceCreator,
    ICohesiveGroup* const templateGroup,
    const string& key) {
  unordered_map<string, shared_ptr<IContextRouter>> routerNodes;
  if (templateGroup == nullptr) {
    return routerNodes;
  }

  const size_t nodeCount = templateGroup->getNodeCount();
  for (size_t i = 0; i < nodeCount; i++) {
    const string nodeName = templateGroup->getNodeName(i);
    const auto groupNodeTemplate = templateGroup->getNode(nodeName);
    const auto routerNode = make_shared<SingleNodeRouter>(
        subinstanceCreator,
        groupNodeTemplate,
        mKey);

    routerNodes[nodeName] = routerNode;
  }

  return routerNodes;
}

size_t CohesiveGroupRouter::getNodeCount() { return mGroupNodeCount; }

string CohesiveGroupRouter::getNodeName(size_t nodeIndex) {
  return mGroupNodeNames[nodeIndex];
}

shared_ptr<INode> CohesiveGroupRouter::getNode(const string& nodeName) {
  auto it = mNodeRouters.find(nodeName);
  if (it == mNodeRouters.end()) {
    return nullptr;
  }

  return it->second->asINode();
}

void CohesiveGroupRouter::addNode(
    const string& contextLookup,
    const shared_ptr<INode>& node) {
  auto group = node->asGroup();
  const size_t subnodeCount = group->getNodeCount();

  for (size_t i = 0; i < subnodeCount; i++) {
    const string subnodeName = group->getNodeName(i);
    const auto subnode = group->getNode(subnodeName);

    const auto& routerNode = mNodeRouters[subnodeName];
    routerNode->addNode(contextLookup, subnode);
  }
}

bool CohesiveGroupRouter::removeNode(const string& contextLookup) {
  auto it = mNodeRouters.find(contextLookup);
  if (it == mNodeRouters.end()) {
    return false;
  }

  for (const auto& nodePair : mNodeRouters) {
    const auto& routerNode = nodePair.second;
    routerNode->removeNode(contextLookup);
  }

  mNodeRouters.erase(it);
  return true;
}

void CohesiveGroupRouter::createNewInstance(const string& forNewContextLookup) {
  const auto subinstanceCreator = mSubinstanceCreator.lock();
  if (!subinstanceCreator) {
    return;
  }

  subinstanceCreator->createNewInstance(forNewContextLookup);
}

ContextualNode::ContextualNode(const json& initData)
    : mImpl(make_shared<Impl>(initData)) {
  mImpl->initialize();  // internal implementation needs a weak pointer, which
                        // it can only get after the constructor.
}

ContextualNode::Impl::Impl(const json& initParameters)
    : mInitParameters(initParameters),
      mNodeImplementation(
          initParameters[kInitDataParameter_NodeImplementation].get<string>()),
      mKey(initParameters[kInitDataParameter_Key].get<string>()) {}

ContextualNode::Impl::~Impl() {}

void ContextualNode::Impl::initialize() {
  const auto templateNode = NodeRegistration::defaultRegistration()->createNode(
      mNodeImplementation,
      mInitParameters);
  mContextRouter = createContextRouter(shared_from_this(), templateNode, mKey);
  mContextRemover = make_shared<ContextRemover>(mContextRouter, mKey);

  mNodeMap[kPartitionName_ContextRouter] = mContextRouter->asINode();
  mNodeMap[kPartitionName_ContextRemover] = mContextRemover;
}

void ContextualNode::Impl::createNewInstance(
    const string& forNewContextLookup) {
  const auto newInstance = NodeRegistration::defaultRegistration()->createNode(
      mNodeImplementation,
      mInitParameters);
  mContextRouter->addNode(forNewContextLookup, newInstance);
}

size_t ContextualNode::getNodeCount() { return mImpl->mNodeMap.size(); }

string ContextualNode::getNodeName(size_t partitionIndex) {
  if (partitionIndex == 0) {
    return kPartitionName_ContextRouter;
  } else if (partitionIndex == 1) {
    return kPartitionName_ContextRemover;
  } else {
    return nullptr;
  }
}

shared_ptr<INode> ContextualNode::getNode(const string& nodeName) {
  auto it = mImpl->mNodeMap.find(nodeName);
  if (it == mImpl->mNodeMap.end()) {
    throw runtime_error("Node '" + nodeName + "' not found.");
  }

  return it->second;
}

}  // namespace maplang
