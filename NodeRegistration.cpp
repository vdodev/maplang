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

#include "NodeRegistration.h"

#include <mutex>

using namespace std;

namespace maplang {

static once_flag createRegistrationOnce;
static NodeRegistration* gDefaultRegistration;

NodeRegistration* NodeRegistration::defaultRegistration() {
  call_once(createRegistrationOnce,
            []() { gDefaultRegistration = new NodeRegistration(); });

  return gDefaultRegistration;
}

void NodeRegistration::registerNodeFactory(const std::string& name,
                                           NodeFactory&& factory) {
  mNodeFactoryMap[name] = move(factory);
}

void NodeRegistration::registerPartitionedNodeFactory(
    const std::string& name, PartitionedNodeFactory&& factory) {
  mPartitionedNodeFactoryMap[name] = move(factory);
}

std::shared_ptr<INode> NodeRegistration::createNode(
    const std::string& name, const nlohmann::json& initParameters) {
  auto it = mNodeFactoryMap.find(name);
  if (it == mNodeFactoryMap.end()) {
    return nullptr;
  }

  NodeFactory& factory = it->second;
  return factory(initParameters);
}

std::shared_ptr<ICohesiveGroup> NodeRegistration::createPartitionedNode(
    const std::string& name, const nlohmann::json& initParameters) {
  auto it = mPartitionedNodeFactoryMap.find(name);
  if (it == mPartitionedNodeFactoryMap.end()) {
    return nullptr;
  }

  PartitionedNodeFactory& factory = it->second;
  return factory(initParameters);
}

void NodeRegistration::visitNodeNames(
    const NodeRegistration::NodeNameVisitor& visitor) {
  for (const auto& pair : mNodeFactoryMap) {
    std::string key = pair.first;
    visitor(key);
  }
}

void NodeRegistration::visitPartitionedNodeNames(
    const NodeRegistration::NodeNameVisitor& visitor) {
  for (const auto& pair : mPartitionedNodeFactoryMap) {
    std::string key = pair.first;
    visitor(key);
  }
}

}  // namespace maplang
