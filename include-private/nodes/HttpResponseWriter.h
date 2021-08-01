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

#ifndef __MAPLANG_RESPONSE_WRITER_H__
#define __MAPLANG_RESPONSE_WRITER_H__

#include "maplang/IImplementation.h"
#include "maplang/ISource.h"

namespace maplang {

class HttpResponseWriter : public IImplementation, public IPathable {
 public:
  HttpResponseWriter(const nlohmann::json& initParameters);
  ~HttpResponseWriter() override = default;

  void handlePacket(const PathablePacket& packet) override;

  ISource* asSource() override { return nullptr; }
  IPathable* asPathable() override { return this; }
  IGroup* asGroup() override { return nullptr; }
};

}  // namespace maplang

#endif  // __MAPLANG_RESPONSE_WRITER_H__
