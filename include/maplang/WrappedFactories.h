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

#ifndef MAPLANG_INCLUDE_MAPLANG_WRAPPEDFACTORIES_H_
#define MAPLANG_INCLUDE_MAPLANG_WRAPPEDFACTORIES_H_

#include "maplang/IFactories.h"

namespace maplang {

class WrappedFactories final : public IFactories {
 public:
  WrappedFactories(const IFactories& factories)
      : mBufferFactory(factories.GetBufferFactory()),
        mImplementationFactory(factories.GetImplementationFactory()),
        mUvLoopRunnerFactory(factories.GetUvLoopRunnerFactory()) {}

  ~WrappedFactories() override = default;

  std::shared_ptr<const IBufferFactory> GetBufferFactory() const override {
    return mBufferFactory;
  }

  std::shared_ptr<const IImplementationFactory> GetImplementationFactory()
      const override {
    return mImplementationFactory.lock();
  }

  std::shared_ptr<const IUvLoopRunnerFactory> GetUvLoopRunnerFactory()
      const override {
    return mUvLoopRunnerFactory;
  }

 private:
  const std::shared_ptr<const IBufferFactory> mBufferFactory;
  std::weak_ptr<const IImplementationFactory> mImplementationFactory;
  const std::shared_ptr<const IUvLoopRunnerFactory> mUvLoopRunnerFactory;
};

}  // namespace maplang

#endif  // MAPLANG_INCLUDE_MAPLANG_WRAPPEDFACTORIES_H_
