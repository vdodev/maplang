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

#ifndef MAPLANG_INCLUDE_MAPLANG_FACTORIESBUILDER_H_
#define MAPLANG_INCLUDE_MAPLANG_FACTORIESBUILDER_H_

#include "maplang/Factories.h"
#include "maplang/IBufferFactory.h"
#include "maplang/IImplementationFactoryBuilder.h"
#include "maplang/IUvLoopRunnerFactory.h"

namespace maplang {

class FactoriesBuilder final {
 public:
  FactoriesBuilder& WithImplementationFactoryBuilder(
      const std::shared_ptr<IImplementationFactoryBuilder>&
          implementationFactoryBuilder);

  FactoriesBuilder& WithBufferFactory(
      const std::shared_ptr<const IBufferFactory>& bufferFactory);

  FactoriesBuilder& WithUvLoopRunnerFactory(
      const std::shared_ptr<const IUvLoopRunnerFactory>& loopRunnerFactory);

  Factories BuildFactories() const;

 private:
  std::optional<std::shared_ptr<IImplementationFactoryBuilder>>
      mImplementationFactoryBuilder;
  std::optional<std::shared_ptr<const IBufferFactory>> mBufferFactory;
  std::optional<std::shared_ptr<const IUvLoopRunnerFactory>>
      mUvLoopRunnerFactory;
};

}  // namespace maplang

#endif  // MAPLANG_INCLUDE_MAPLANG_FACTORIESBUILDER_H_
