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

#include <mutex>

#include "maplang/json.hpp"
#include "nodes/ContextualNode.h"
#include "nodes/HttpRequestExtractor.h"
#include "nodes/HttpRequestHeaderWriter.h"
#include "nodes/HttpResponseExtractor.h"
#include "nodes/HttpResponseWriter.h"
#include "nodes/OrderedPacketSender.h"
#include "nodes/SendOnce.h"
#include "nodes/UvTcpConnectionGroup.h"
#include "nodes/VolatileKeyValueSet.h"
#include "nodes/VolatileKeyValueStore.h"

using namespace std;
using namespace nlohmann;

namespace maplang {

static once_flag createRegistrationOnce;
static NodeRegistration* gDefaultRegistration;

static void registerNodes(NodeRegistration* registration) {
  registration->registerNodeFactory("Contextual Node", [](const nlohmann::json& initParameters) {
    return make_shared<ContextualNode>(initParameters);
  });

  registration->registerNodeFactory("TCP Connection", [](const nlohmann::json& initParameters) {
    return make_shared<UvTcpConnectionGroup>();
  });

  registration->registerNodeFactory("HTTP Request Header Writer", [](const json& initParameters) {
    return make_shared<HttpRequestHeaderWriter>(initParameters);
  });

  registration->registerNodeFactory("HTTP Request Extractor", [](const json& initParameters) {
    return make_shared<HttpRequestExtractor>(initParameters);
  });

  registration->registerNodeFactory("Send Once", [](const json& initParameters) {
    return make_shared<SendOnce>(initParameters);
  });

  registration->registerNodeFactory("HTTP Response Writer", [](const json& initParameters) {
    return make_shared<HttpResponseWriter>(initParameters);
  });

  registration->registerNodeFactory("HTTP Response Extractor", [](const json& initParameters) {
    return make_shared<HttpResponseExtractor>(initParameters);
  });

  registration->registerNodeFactory("Volatile Key Value Store", [](const json& initParameters) {
    return make_shared<VolatileKeyValueStore>(initParameters);
  });

  registration->registerNodeFactory("Volatile Key Value Set", [](const json& initParameters) {
    return make_shared<VolatileKeyValueSet>(initParameters);
  });

  registration->registerNodeFactory("Ordered Packet Sender", [](const json& initParameters) {
    return make_shared<OrderedPacketSender>();
  });
}

NodeRegistration* NodeRegistration::defaultRegistration() {
  call_once(createRegistrationOnce, []() {
    gDefaultRegistration = new NodeRegistration();
    registerNodes(gDefaultRegistration);
  });

  return gDefaultRegistration;
}

void NodeRegistration::registerNodeFactory(const std::string& name, NodeFactory&& factory) {
  mNodeFactoryMap[name] = move(factory);
}

std::shared_ptr<INode> NodeRegistration::createNode(const std::string& name, const nlohmann::json& initParameters) {
  auto it = mNodeFactoryMap.find(name);
  if (it == mNodeFactoryMap.end()) {
    throw runtime_error("No Node factory exists for node type '" + name + "'.");
  }

  NodeFactory& factory = it->second;
  return factory(initParameters);
}

void NodeRegistration::visitNodeNames(const NodeRegistration::NodeNameVisitor& visitor) {
  for (const auto& pair : mNodeFactoryMap) {
    std::string key = pair.first;
    visitor(key);
  }
}

}  // namespace maplang
