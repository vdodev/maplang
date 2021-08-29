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

#ifndef MAPLANG_IMPLEMENTATIONFACTORY_H_
#define MAPLANG_IMPLEMENTATIONFACTORY_H_

#include <future>
#include <memory>
#include <set>

#include "maplang/IBufferFactory.h"
#include "maplang/IGroup.h"
#include "maplang/IImplementation.h"
#include "maplang/IImplementationFactory.h"
#include "maplang/IUvLoopRunnerFactory.h"
#include "maplang/json.hpp"

namespace maplang {

class ImplementationFactory final
    : public IImplementationFactory,
      public std::enable_shared_from_this<ImplementationFactory> {
 public:
  static std::shared_ptr<ImplementationFactory> Create(
      const std::shared_future<Factories>&
          factoriesFutureWhichDeadlocksInConstructor);

  ~ImplementationFactory() override = default;

  void registerFactory(const std::string& name, const FactoryFunction& factory);

  std::shared_ptr<IImplementation> createImplementation(
      const std::string& name,
      const nlohmann::json& initParameters) const override;

  void visitImplementationNames(
      const ImplementationNameVisitor& visitor) const override;

 private:
  std::shared_future<Factories> mFactoriesFutureWhichDeadlocksInConstructor;

  const std::shared_ptr<const IBufferFactory> mBufferFactory;
  const std::shared_ptr<const IUvLoopRunnerFactory> mUvLoopRunnerFactory;

  std::unordered_map<std::string, FactoryFunction> mFactoryFunctionMap;

 private:
  explicit ImplementationFactory(
      const std::shared_future<Factories>&
          factoriesFutureWhichDeadlocksInConstructor);

  void RegisterImplementations();
};

}  // namespace maplang

#endif  // MAPLANG_IMPLEMENTATIONFACTORY_H_
