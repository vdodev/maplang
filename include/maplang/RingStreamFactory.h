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
#ifndef MAPLANG__RINGSTREAMFACTORY_H_
#define MAPLANG__RINGSTREAMFACTORY_H_

#include <stddef.h>
#include <stdint.h>

#include <functional>

#include "Buffer.h"
#include "IBufferFactory.h"
#include "IRingStreamFactory.h"

namespace maplang {

class RingStreamFactory final : public IRingStreamFactory {
 public:
  RingStreamFactory(
      const std::shared_ptr<const IBufferFactory>& bufferFactory,
      std::optional<size_t> initialRingBufferSize = {});

  std::shared_ptr<RingStream> Create() const override;

 private:
  const std::shared_ptr<const IBufferFactory> mBufferFactory;
  std::optional<size_t> mInitialSize;
};

}  // namespace maplang

#endif  // MAPLANG__RINGSTREAMFACTORY_H_
