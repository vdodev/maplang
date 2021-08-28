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
#include "maplang/ImplementationFactory.h"

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
#include "nodes/ParameterRouter.h"
#include "nodes/PassThroughNode.h"
#include "nodes/SendOnce.h"
#include "nodes/UvTcpConnectionGroup.h"
#include "nodes/VolatileKeyValueSet.h"
#include "nodes/VolatileKeyValueStore.h"
#include "nodes/BufferAccumulatorNode.h"

using namespace std;
using namespace nlohmann;

namespace maplang {

static once_flag createDefaultFactoryOnce;
static shared_ptr<ImplementationFactory> gDefaultFactory;

static void registerImplementations(
    const shared_ptr<ImplementationFactory>& factory) {
  factory->registerFactory("Pass-through", [](const json& initParams) {
    return make_shared<PassThroughNode>(initParams);
  });

  factory->registerFactory(
             "Buffer Accumulator",
             [](const nlohmann::json& initParams) {
               return make_shared<BufferAccumulatorNode>(initParams);
             });

  factory->registerFactory(
      "Add Parameters",
      [](const nlohmann::json& initParams) {
        return make_shared<AddParametersNode>(initParams);
      });

  factory->registerFactory(
      "Parameter Extractor",
      [](const nlohmann::json& initParameters) {
        return make_shared<ParameterExtractor>(initParameters);
      });

  factory->registerFactory(
      "Parameter Router",
      [](const nlohmann::json& initParameters) {
        return make_shared<ParameterRouter>(initParameters);
      });

  factory->registerFactory(
      "Contextual",
      [](const nlohmann::json& initParameters) {
        return make_shared<ContextualNode>(initParameters);
      });

  factory->registerFactory(
      "TCP Server",
      [](const nlohmann::json& initParameters) {
        return make_shared<UvTcpConnectionGroup>();
      });

  factory->registerFactory(
      "HTTP Request Header Writer",
      [](const json& initParameters) {
        return make_shared<HttpRequestHeaderWriter>(initParameters);
      });

  factory->registerFactory(
      "HTTP Request Extractor",
      [](const json& initParameters) {
        return make_shared<HttpRequestExtractor>(initParameters);
      });

  factory->registerFactory("Send Once", [](const json& initParameters) {
    return make_shared<SendOnce>(initParameters);
  });

  factory->registerFactory(
      "HTTP Response Writer",
      [](const json& initParameters) {
        return make_shared<HttpResponseWriter>(initParameters);
      });

  factory->registerFactory(
      "HTTP Response Extractor",
      [](const json& initParameters) {
        return make_shared<HttpResponseExtractor>(initParameters);
      });

  factory->registerFactory(
      "Volatile Key Value Store",
      [](const json& initParameters) {
        return make_shared<VolatileKeyValueStore>(initParameters);
      });

  factory->registerFactory(
      "Volatile Key Value Set",
      [](const json& initParameters) {
        return make_shared<VolatileKeyValueSet>(initParameters);
      });

  factory->registerFactory(
      "Ordered Packet Sender",
      [](const json& initParameters) {
        return make_shared<OrderedPacketSender>();
      });
}

shared_ptr<ImplementationFactory> ImplementationFactory::defaultFactory() {
  call_once(createDefaultFactoryOnce, []() {
    gDefaultFactory = make_shared<ImplementationFactory>();
    registerImplementations(gDefaultFactory);
  });

  return gDefaultFactory;
}

void ImplementationFactory::registerFactory(
    const std::string& name,
    FactoryFunction&& factory) {
  mFactoryFunctionMap[name] = move(factory);
}

std::shared_ptr<IImplementation> ImplementationFactory::createImplementation(
    const std::string& name,
    const nlohmann::json& initParameters) const {
  auto it = mFactoryFunctionMap.find(name);
  if (it == mFactoryFunctionMap.end()) {
    throw runtime_error("No factory exists for implementation '" + name + "'.");
  }

  const FactoryFunction& factory = it->second;
  return factory(initParameters);
}

void ImplementationFactory::visitImplementationNames(
    const ImplementationFactory::ImplementationNameVisitor& visitor) {
  for (const auto& pair : mFactoryFunctionMap) {
    std::string key = pair.first;
    visitor(key);
  }
}

}  // namespace maplang
