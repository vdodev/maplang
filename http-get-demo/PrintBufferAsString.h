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

#ifndef MAPLANG_PRINTBUFFERASSTRING_H_
#define MAPLANG_PRINTBUFFERASSTRING_H_

#include "maplang/IImplementation.h"
#include "maplang/IPathable.h"

namespace maplang {

class PrintBufferAsString final : public INode, public ISink {
 public:
  ~PrintBufferAsString() override = default;

  void handlePacket(const Packet& incomingPacket) override;

  IPathable* asPathable() override { return nullptr; }
  ISource* asSource() override { return nullptr; }
  ISink* asSink() override { return this; }
};

}  // namespace maplang

#endif  // MAPLANG_HTTP_RESPONSE_WITH_ADDRESS_AS_BODY_H_
