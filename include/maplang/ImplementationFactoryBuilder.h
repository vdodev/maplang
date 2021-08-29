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

#ifndef MAPLANG_INCLUDE_MAPLANG_IMPLEMENTATIONFACTORYBUILDER_H_
#define MAPLANG_INCLUDE_MAPLANG_IMPLEMENTATIONFACTORYBUILDER_H_

#include "maplang/IImplementationFactoryBuilder.h"

namespace maplang {

class ImplementationFactoryBuilder final
    : public IImplementationFactoryBuilder {
 public:
  ImplementationFactoryBuilder& WithFactoryForName(
      const std::string& name,
      const IImplementationFactory::FactoryFunction& implementationFactory);

  std::shared_ptr<const IImplementationFactory> BuildImplementationFactory(
      const std::shared_future<const Factories>&
          factoriesFutureWhichDeadlocksInConstructor) const override;

 private:
  std::unordered_map<std::string, IImplementationFactory::FactoryFunction>
      mFactoryFunctions;
};

}  // namespace maplang

#endif  // MAPLANG_INCLUDE_MAPLANG_IMPLEMENTATIONFACTORYBUILDER_H_
