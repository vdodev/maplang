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

#ifndef MAPLANG_INCLUDE_MAPLANG_IIMPLEMENTATIONFACTORYBUILDER_H_
#define MAPLANG_INCLUDE_MAPLANG_IIMPLEMENTATIONFACTORYBUILDER_H_

#include <future>
#include <memory>

#include "maplang/IBufferFactory.h"
#include "maplang/IImplementationFactory.h"
#include "maplang/IUvLoopRunnerFactory.h"

namespace maplang {

class IImplementationFactoryBuilder {
 public:
  virtual ~IImplementationFactoryBuilder() = default;

  virtual std::shared_ptr<const IImplementationFactory>
  BuildImplementationFactory(
      const std::shared_future<const Factories>&
          factoriesFutureWhichDeadlocksInConstructor) const = 0;
};

}  // namespace maplang

#endif  // MAPLANG_INCLUDE_MAPLANG_IIMPLEMENTATIONFACTORYBUILDER_H_
