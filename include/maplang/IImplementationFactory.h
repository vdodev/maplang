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

#ifndef __MAPLANG_IIMPLEMENTATIONFACTORY_H__
#define __MAPLANG_IIMPLEMENTATIONFACTORY_H__

#include <memory>
#include <set>

#include "maplang/IGroup.h"
#include "maplang/IImplementation.h"
#include "maplang/json.hpp"

namespace maplang {

class IFactories;

class IImplementationFactory {
 public:
  using FactoryFunction = std::function<std::shared_ptr<IImplementation>(
      const IFactories& factories,
      const nlohmann::json& initParameters)>;

  using ImplementationNameVisitor =
      std::function<void(const std::string& nodeName)>;

 public:
  virtual ~IImplementationFactory() = default;

  virtual std::shared_ptr<IImplementation> createImplementation(
      const std::string& name,
      const nlohmann::json& initParameters) const = 0;

  virtual void visitImplementationNames(
      const ImplementationNameVisitor& visitor) const = 0;
};

}  // namespace maplang

#endif  // __MAPLANG_IIMPLEMENTATIONFACTORY_H__
