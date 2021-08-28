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

#include "maplang/Factories.h"
#include "maplang/json.hpp"
#include "maplang/stream-util.h"
#include "nodes/AddParametersNode.h"
#include "nodes/BufferAccumulatorNode.h"
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

using namespace std;
using namespace nlohmann;

namespace maplang {

std::shared_ptr<ImplementationFactory> ImplementationFactory::Create(
    const std::shared_ptr<const IBufferFactory>& bufferFactory,
    const std::shared_ptr<const IUvLoopRunnerFactory>& uvLoopRunnerFactory) {
  return shared_ptr<ImplementationFactory>(
      new ImplementationFactory(bufferFactory, uvLoopRunnerFactory));
}

ImplementationFactory::ImplementationFactory(
    const shared_ptr<const IBufferFactory>& bufferFactory,
    const shared_ptr<const IUvLoopRunnerFactory>& uvLoopRunnerFactory)
    : mBufferFactory(bufferFactory), mUvLoopRunnerFactory(uvLoopRunnerFactory) {
  RegisterImplementations();
}

void ImplementationFactory::RegisterImplementations() {
  registerFactory(
      "Pass-through",
      [](const IFactories& factories,
         const nlohmann::json& initParameters) {
        return make_shared<PassThroughNode>(initParameters);
      });

  registerFactory(
      "Buffer Accumulator",
      [](const IFactories& factories,
         const nlohmann::json& initParameters) {
        return make_shared<BufferAccumulatorNode>(initParameters);
      });

  registerFactory(
      "Add Parameters",
      [](const IFactories& factories,
         const nlohmann::json& initParameters) {
        return make_shared<AddParametersNode>(initParameters);
      });

  registerFactory(
      "Parameter Extractor",
      [](const IFactories& factories,
         const nlohmann::json& initParameters) {
        return make_shared<ParameterExtractor>(initParameters);
      });

  registerFactory(
      "Parameter Router",
      [](const IFactories& factories,
         const nlohmann::json& initParameters) {
        return make_shared<ParameterRouter>(initParameters);
      });

  registerFactory(
      "Contextual",
      [](const IFactories& factories,
         const nlohmann::json& initParameters) {
        return make_shared<ContextualNode>(factories, initParameters);
      });

  registerFactory(
      "TCP Server",
      [](const IFactories& factories,
         const nlohmann::json& initParameters) {
        return make_shared<UvTcpConnectionGroup>();
      });

  registerFactory(
      "HTTP Request Header Writer",
      [](const IFactories& factories,
         const nlohmann::json& initParameters) {
        return make_shared<HttpRequestHeaderWriter>(initParameters);
      });

  registerFactory(
      "HTTP Request Extractor",
      [](const IFactories& factories,
         const nlohmann::json& initParameters) {
        return make_shared<HttpRequestExtractor>(initParameters);
      });

  registerFactory(
      "Send Once",
      [](const IFactories& factories,
         const nlohmann::json& initParameters) {
        return make_shared<SendOnce>(initParameters);
      });

  registerFactory(
      "HTTP Response Writer",
      [](const IFactories& factories,
         const nlohmann::json& initParameters) {
        return make_shared<HttpResponseWriter>(initParameters);
      });

  registerFactory(
      "HTTP Response Extractor",
      [](const IFactories& factories,
         const nlohmann::json& initParameters) {
        return make_shared<HttpResponseExtractor>(initParameters);
      });

  registerFactory(
      "Volatile Key Value Store",
      [](const IFactories& factories,
         const nlohmann::json& initParameters) {
        return make_shared<VolatileKeyValueStore>(initParameters);
      });

  registerFactory(
      "Volatile Key Value Set",
      [](const IFactories& factories,
         const nlohmann::json& initParameters) {
        return make_shared<VolatileKeyValueSet>(initParameters);
      });

  registerFactory(
      "Ordered Packet Sender",
      [](const IFactories& factories,
         const nlohmann::json& initParameters) {
        return make_shared<OrderedPacketSender>();
      });
}

void ImplementationFactory::registerFactory(
    const std::string& name,
    const FactoryFunction& factory) {
  mFactoryFunctionMap[name] = move(factory);
}

std::shared_ptr<IImplementation> ImplementationFactory::createImplementation(
    const std::string& name,
    const nlohmann::json& initParameters) const {
  Factories factories(
      mBufferFactory,
      shared_from_this(),
      mUvLoopRunnerFactory);

  auto it = mFactoryFunctionMap.find(name);
  if (it == mFactoryFunctionMap.end()) {
    throw runtime_error("No factory exists for implementation '" + name + "'.");
  }

  const FactoryFunction& nodeImplementationFactory = it->second;

  return nodeImplementationFactory(factories, initParameters);
}

void ImplementationFactory::visitImplementationNames(
    const ImplementationFactory::ImplementationNameVisitor& visitor) const {
  for (const auto& pair : mFactoryFunctionMap) {
    std::string key = pair.first;
    visitor(key);
  }
}

}  // namespace maplang
