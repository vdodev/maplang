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

#include "maplang/ImplementationFactoryBuilder.h"

#include "maplang/BufferFactory.h"
#include "maplang/ImplementationFactory.h"
#include "maplang/WrappedFactories.h"

using namespace std;

namespace maplang {

ImplementationFactoryBuilder& ImplementationFactoryBuilder::WithFactoryForName(
    const std::string& name,
    const IImplementationFactory::FactoryFunction& implementationFactory) {
  mFactoryFunctions.emplace(name, implementationFactory);

  return *this;
}

std::shared_ptr<const IImplementationFactory>
ImplementationFactoryBuilder::BuildImplementationFactory(
    const std::shared_ptr<const IBufferFactory>& bufferFactory,
    const std::shared_ptr<const IUvLoopRunnerFactory>& uvRunnerFactory) const {
  const auto implementationFactory =
      ImplementationFactory::Create(bufferFactory, uvRunnerFactory);

  for (auto& [name, factory] : mFactoryFunctions) {
    implementationFactory->registerFactory(name, std::move(factory));
  }

  return implementationFactory;
}

}  // namespace maplang
