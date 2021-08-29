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

std::shared_ptr<const ImplementationFactory> ImplementationFactory::Create(
    const std::shared_future<const Factories>&
        factoriesFutureWhichDeadlocksInConstructor,
    const std::unordered_map<std::string, FactoryFunction>& factoryFunctions) {
  const auto implementationFactory = shared_ptr<ImplementationFactory>(
      new ImplementationFactory(factoriesFutureWhichDeadlocksInConstructor));

  for (const auto& [name, factory] : factoryFunctions) {
    implementationFactory->registerFactory(name, factory);
  }

  return implementationFactory;
}

ImplementationFactory::ImplementationFactory(
    const std::shared_future<const Factories>&
        factoriesFutureWhichDeadlocksInConstructor)
    : mFactoriesFutureWhichDeadlocksInConstructor(
        factoriesFutureWhichDeadlocksInConstructor) {
  RegisterImplementations();
}

void ImplementationFactory::RegisterImplementations() {
  registerFactory(
      "Pass-through",
      [](const Factories& factories, const nlohmann::json& initParameters) {
        return make_shared<PassThroughNode>(initParameters);
      });

  registerFactory(
      "Buffer Accumulator",
      [](const Factories& factories, const nlohmann::json& initParameters) {
        return make_shared<BufferAccumulatorNode>(factories, initParameters);
      });

  registerFactory(
      "Add Parameters",
      [](const Factories& factories, const nlohmann::json& initParameters) {
        return make_shared<AddParametersNode>(factories, initParameters);
      });

  registerFactory(
      "Parameter Extractor",
      [](const Factories& factories, const nlohmann::json& initParameters) {
        return make_shared<ParameterExtractor>(factories, initParameters);
      });

  registerFactory(
      "Parameter Router",
      [](const Factories& factories, const nlohmann::json& initParameters) {
        return make_shared<ParameterRouter>(factories, initParameters);
      });

  registerFactory(
      "Contextual",
      [](const Factories& factories, const nlohmann::json& initParameters) {
        return make_shared<ContextualNode>(factories, initParameters);
      });

  registerFactory(
      "TCP Server",
      [](const Factories& factories, const nlohmann::json& initParameters) {
        return make_shared<UvTcpConnectionGroup>(factories, initParameters);
      });

  registerFactory(
      "HTTP Request Header Writer",
      [](const Factories& factories, const nlohmann::json& initParameters) {
        return make_shared<HttpRequestHeaderWriter>(factories, initParameters);
      });

  registerFactory(
      "HTTP Request Extractor",
      [](const Factories& factories, const nlohmann::json& initParameters) {
        return make_shared<HttpRequestExtractor>(factories, initParameters);
      });

  registerFactory(
      "Send Once",
      [](const Factories& factories, const nlohmann::json& initParameters) {
        return make_shared<SendOnce>(factories, initParameters);
      });

  registerFactory(
      "HTTP Response Writer",
      [](const Factories& factories, const nlohmann::json& initParameters) {
        return make_shared<HttpResponseWriter>(factories, initParameters);
      });

  registerFactory(
      "HTTP Response Extractor",
      [](const Factories& factories, const nlohmann::json& initParameters) {
        return make_shared<HttpResponseExtractor>(factories, initParameters);
      });

  registerFactory(
      "Volatile Key Value Store",
      [](const Factories& factories, const nlohmann::json& initParameters) {
        return make_shared<VolatileKeyValueStore>(factories, initParameters);
      });

  registerFactory(
      "Volatile Key Value Set",
      [](const Factories& factories, const nlohmann::json& initParameters) {
        return make_shared<VolatileKeyValueSet>(factories, initParameters);
      });

  registerFactory(
      "Ordered Packet Sender",
      [](const Factories& factories, const nlohmann::json& initParameters) {
        return make_shared<OrderedPacketSender>(factories, initParameters);
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
  auto it = mFactoryFunctionMap.find(name);
  if (it == mFactoryFunctionMap.end()) {
    throw runtime_error("No factory exists for implementation '" + name + "'.");
  }

  const FactoryFunction& nodeImplementationFactory = it->second;

  return nodeImplementationFactory(
      mFactoriesFutureWhichDeadlocksInConstructor.get(),
      initParameters);
}

void ImplementationFactory::visitImplementationNames(
    const ImplementationFactory::ImplementationNameVisitor& visitor) const {
  for (const auto& pair : mFactoryFunctionMap) {
    std::string key = pair.first;
    visitor(key);
  }
}

}  // namespace maplang
