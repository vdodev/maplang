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

#include "maplang/Instance.h"

#include "maplang/INode.h"
#include "maplang/NodeFactory.h"

using namespace std;

namespace maplang {

void Instance::setType(
    const string& typeName,
    const std::shared_ptr<NodeFactory>& nodeFactory) {
  if (mTypeName == typeName) {
    return;
  }

  mImplementation.reset();

  if (typeName == "") {
    return;
  }

  mImplementation = nodeFactory->createNode(typeName, mInitParameters);

  const auto source = mImplementation->asSource();
  const auto packetPusherForISources = mPacketPusherForISources.lock();
  if (source != nullptr && packetPusherForISources != nullptr) {
    source->setPacketPusher(packetPusherForISources);
  }

  mTypeName = typeName;
}

void Instance::setImplementation(const shared_ptr<INode>& implementation) {
  mImplementation = implementation;

  const auto source = mImplementation->asSource();
  const auto packetPusherForISources = mPacketPusherForISources.lock();
  if (source != nullptr && packetPusherForISources != nullptr) {
    source->setPacketPusher(packetPusherForISources);
  }
}

void Instance::setInitParameters(const nlohmann::json& initParameters) {
  mInitParameters = initParameters;
}

void Instance::setSubgraphContext(
    const std::shared_ptr<ISubgraphContext>& subgraphContext) {
  mSubgraphContext = subgraphContext;

  if (mImplementation != nullptr) {
    mImplementation->setSubgraphContext(mSubgraphContext);
  }
}

std::shared_ptr<ISubgraphContext> Instance::getSubgraphContext() const {
  return mSubgraphContext;
}

void Instance::setThreadGroupName(const string& threadGroupName) {
  mThreadGroupName = threadGroupName;
}

string Instance::getThreadGroupName() const { return mThreadGroupName; }

void Instance::setPacketPusherForISources(
    const std::shared_ptr<IPacketPusher>& packetPusher) {
  const auto currentPacketPusher = mPacketPusherForISources.lock();
  if (currentPacketPusher != nullptr && packetPusher != nullptr) {
    throw runtime_error(
        "Setting multiple packet pushers for an ISource is not supported. An "
        "Instance with an ISource implementation is probably referenced in "
        "more than one GraphElement.");
  }

  mPacketPusherForISources = packetPusher;

  if (mImplementation == nullptr) {
    return;
  }

  const auto source = mImplementation->asSource();
  if (source != nullptr && packetPusher != nullptr) {
    source->setPacketPusher(packetPusher);
  }
}

}  // namespace maplang
