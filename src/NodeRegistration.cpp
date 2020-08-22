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

#include "maplang/NodeRegistration.h"
#include "maplang/json.hpp"
#include "nodes/VolatileKeyValueStore.h"
#include "nodes/VolatileKeyValueSet.h"
#include "nodes/HttpRequestExtractor.h"
#include "nodes/HttpResponseWriter.h"
#include "nodes/SendOnce.h"
#include "nodes/ContextualNode.h"
#include "nodes/tcp/UvTcpServerGroup.h"
#include <mutex>

using namespace std;
using namespace nlohmann;

namespace maplang {

static once_flag createRegistrationOnce;
static NodeRegistration* gDefaultRegistration;

static void registerNodes(NodeRegistration* registration) {
  registration->registerCohesiveGroupFactory(
      "Contextual Node",
      [](const nlohmann::json& initParameters) {
        return make_shared<ContextualNode>(initParameters);
      });

  registration->registerCohesiveGroupFactory(
      "TCP Server",
      [](const nlohmann::json &initParameters) {
        return make_shared<UvTcpServerGroup>();
      });

  registration->registerNodeFactory(
      "HTTP Request Extractor",
      [](const json& initParameters) {
        return make_shared<HttpRequestExtractor>(initParameters);
      });

  registration->registerNodeFactory(
      "Send Once",
      [](const json& initParameters) {
        return make_shared<SendOnce>(initParameters);
      });

  registration->registerNodeFactory(
      "HTTP Response Writer",
      [](const json& initParameters) {
        return make_shared<HttpResponseWriter>();
      });

  registration->registerCohesiveGroupFactory(
      "Volatile Key Value Store",
      [](const json& initParameters) {
        return make_shared<VolatileKeyValueStore>(initParameters);
      });

  registration->registerCohesiveGroupFactory(
      "Volatile Key Value Set",
      [](const json& initParameters) {
        return make_shared<VolatileKeyValueSet>(initParameters);
      });
}

NodeRegistration* NodeRegistration::defaultRegistration() {
  call_once(createRegistrationOnce,
            []() {
    gDefaultRegistration = new NodeRegistration();
    registerNodes(gDefaultRegistration);
  });

  return gDefaultRegistration;
}

void NodeRegistration::registerNodeFactory(const std::string& name,
                                           NodeFactory&& factory) {
  mNodeFactoryMap[name] = move(factory);
}

void NodeRegistration::registerCohesiveGroupFactory(
    const std::string& name, CohesiveGroupFactory&& factory) {
  mCohesiveGroupFactoryMap[name] = move(factory);
}

std::shared_ptr<INode> NodeRegistration::createNode(
    const std::string& name, const nlohmann::json& initParameters) {
  auto it = mNodeFactoryMap.find(name);
  if (it == mNodeFactoryMap.end()) {
    throw runtime_error("No Node factory exists for node type '" + name + "'.");
  }

  NodeFactory& factory = it->second;
  return factory(initParameters);
}

std::shared_ptr<ICohesiveGroup> NodeRegistration::createCohesiveGroup(
    const std::string& name, const nlohmann::json& initParameters) {
  auto it = mCohesiveGroupFactoryMap.find(name);
  if (it == mCohesiveGroupFactoryMap.end()) {
    throw runtime_error("No CohesiveGroup factory exists for type '" + name + "'.");
  }

  CohesiveGroupFactory& factory = it->second;
  return factory(initParameters);
}

void NodeRegistration::visitNodeNames(
    const NodeRegistration::NodeNameVisitor& visitor) {
  for (const auto& pair : mNodeFactoryMap) {
    std::string key = pair.first;
    visitor(key);
  }
}

void NodeRegistration::visitCohesiveGroupNames(
    const NodeRegistration::NodeNameVisitor& visitor) {
  for (const auto& pair : mCohesiveGroupFactoryMap) {
    std::string key = pair.first;
    visitor(key);
  }
}

}  // namespace maplang
