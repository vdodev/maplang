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

#include "maplang/FactoriesBuilder.h"

#include <future>

#include "maplang/BufferFactory.h"
#include "maplang/Factories.h"
#include "maplang/ImplementationFactory.h"
#include "maplang/ImplementationFactoryBuilder.h"
#include "maplang/UvLoopRunnerFactory.h"

using namespace std;

namespace maplang {

FactoriesBuilder& FactoriesBuilder::WithBufferFactory(
    const std::shared_ptr<const IBufferFactory>& bufferFactory) {
  mBufferFactory = bufferFactory;

  return *this;
}

FactoriesBuilder& FactoriesBuilder::WithImplementationFactoryBuilder(
    const std::shared_ptr<IImplementationFactoryBuilder>&
        implementationFactoryBuilder) {
  mImplementationFactoryBuilder = implementationFactoryBuilder;

  return *this;
}

Factories FactoriesBuilder::BuildFactories() const {
  const shared_ptr<const IBufferFactory> bufferFactory =
      mBufferFactory ? *mBufferFactory : make_shared<BufferFactory>();

  const shared_ptr<IImplementationFactoryBuilder> implementationFactoryBuilder =
      mImplementationFactoryBuilder
          ? *mImplementationFactoryBuilder
          : make_shared<ImplementationFactoryBuilder>();

  const shared_ptr<const IUvLoopRunnerFactory> uvLoopRunnerFactory =
      mUvLoopRunnerFactory ? *mUvLoopRunnerFactory
                           : make_shared<UvLoopRunnerFactory>();

  std::promise<const Factories> factoriesPromise;

  const Factories factories(
      bufferFactory,
      implementationFactoryBuilder->BuildImplementationFactory(
          factoriesPromise.get_future()),
      uvLoopRunnerFactory);

  factoriesPromise.set_value(factories);

  return factories;
}

}  // namespace maplang
