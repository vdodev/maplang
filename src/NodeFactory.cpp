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
#include "maplang/NodeFactory.h"

#include <mutex>

#include "maplang/json.hpp"
#include "nodes/AddParametersNode.h"
#include "nodes/ContextualNode.h"
#include "nodes/HttpRequestExtractor.h"
#include "nodes/HttpRequestHeaderWriter.h"
#include "nodes/HttpResponseExtractor.h"
#include "nodes/HttpResponseWriter.h"
#include "nodes/OrderedPacketSender.h"
#include "nodes/ParameterExtractor.h"
#include "nodes/PassThroughNode.h"
#include "nodes/SendOnce.h"
#include "nodes/UvTcpConnectionGroup.h"
#include "nodes/VolatileKeyValueSet.h"
#include "nodes/VolatileKeyValueStore.h"

using namespace std;
using namespace nlohmann;

namespace maplang {

static once_flag createNodeFactoryOnce;
static shared_ptr<NodeFactory> gDefaultFactory;

static void registerNodes(const shared_ptr<NodeFactory>& factory) {
  factory->registerNodeFactory("Pass-through", [](const json& initParams) {
    return make_shared<PassThroughNode>(initParams);
  });

  factory->registerNodeFactory(
      "Add Parameters",
      [](const nlohmann::json& initParams) {
        return make_shared<AddParametersNode>(initParams);
      });

  factory->registerNodeFactory(
      "Parameter Extractor",
      [](const nlohmann::json& initParameters) {
        return make_shared<ParameterExtractor>(initParameters);
      });

  factory->registerNodeFactory(
      "Contextual Node",
      [](const nlohmann::json& initParameters) {
        return make_shared<ContextualNode>(initParameters);
      });

  factory->registerNodeFactory(
      "TCP Connection",
      [](const nlohmann::json& initParameters) {
        return make_shared<UvTcpConnectionGroup>();
      });

  factory->registerNodeFactory(
      "HTTP Request Header Writer",
      [](const json& initParameters) {
        return make_shared<HttpRequestHeaderWriter>(initParameters);
      });

  factory->registerNodeFactory(
      "HTTP Request Extractor",
      [](const json& initParameters) {
        return make_shared<HttpRequestExtractor>(initParameters);
      });

  factory->registerNodeFactory("Send Once", [](const json& initParameters) {
    return make_shared<SendOnce>(initParameters);
  });

  factory->registerNodeFactory(
      "HTTP Response Writer",
      [](const json& initParameters) {
        return make_shared<HttpResponseWriter>(initParameters);
      });

  factory->registerNodeFactory(
      "HTTP Response Extractor",
      [](const json& initParameters) {
        return make_shared<HttpResponseExtractor>(initParameters);
      });

  factory->registerNodeFactory(
      "Volatile Key Value Store",
      [](const json& initParameters) {
        return make_shared<VolatileKeyValueStore>(initParameters);
      });

  factory->registerNodeFactory(
      "Volatile Key Value Set",
      [](const json& initParameters) {
        return make_shared<VolatileKeyValueSet>(initParameters);
      });

  factory->registerNodeFactory(
      "Ordered Packet Sender",
      [](const json& initParameters) {
        return make_shared<OrderedPacketSender>();
      });
}

shared_ptr<NodeFactory> NodeFactory::defaultFactory() {
  call_once(createNodeFactoryOnce, []() {
    gDefaultFactory = make_shared<NodeFactory>();
    registerNodes(gDefaultFactory);
  });

  return gDefaultFactory;
}

void NodeFactory::registerNodeFactory(
    const std::string& name,
    NodeFactoryFunction&& factory) {
  mNodeFactoryFunctionMap[name] = move(factory);
}

std::shared_ptr<INode> NodeFactory::createNode(
    const std::string& name,
    const nlohmann::json& initParameters) const {
  auto it = mNodeFactoryFunctionMap.find(name);
  if (it == mNodeFactoryFunctionMap.end()) {
    throw runtime_error(
        "No Node factory exists for node implementation '" + name + "'.");
  }

  const NodeFactoryFunction& factory = it->second;
  return factory(initParameters);
}

void NodeFactory::visitNodeNames(const NodeFactory::NodeNameVisitor& visitor) {
  for (const auto& pair : mNodeFactoryFunctionMap) {
    std::string key = pair.first;
    visitor(key);
  }
}

}  // namespace maplang
