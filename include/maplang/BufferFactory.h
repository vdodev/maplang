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
#ifndef MAPLANG_BUFFERFACTORY_H_
#define MAPLANG_BUFFERFACTORY_H_

#include "IBufferFactory.h"

namespace maplang {

class BufferFactory final : public IBufferFactory {
 public:
  Buffer Create(size_t bufferSize) const override {
    return Buffer(
        std::shared_ptr<uint8_t>(
            new uint8_t[bufferSize],
            std::default_delete<uint8_t[]>()),
        bufferSize);
  }
};

}  // namespace maplang
#endif  // MAPLANG_BUFFERFACTORY_H_
