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

#ifndef MAPLANG_HTTP_RESPONSE_WITH_ADDRESS_AS_BODY_H_
#define MAPLANG_HTTP_RESPONSE_WITH_ADDRESS_AS_BODY_H_

#include "maplang/Factories.h"
#include "maplang/IImplementation.h"
#include "maplang/IPathable.h"

namespace maplang {

class HttpResponseWithAddressAsBody final : public IImplementation,
                                            public IPathable {
 public:
  HttpResponseWithAddressAsBody(
      const Factories& factories,
      const nlohmann::json& initParameters);
  ~HttpResponseWithAddressAsBody() override = default;

  void handlePacket(const PathablePacket& incomingPacket) override;

  IPathable* asPathable() override { return this; }
  ISource* asSource() override { return nullptr; }
  IGroup* asGroup() override { return nullptr; }

 private:
  const Factories mFactories;
  const nlohmann::json mInitParameters;
};

}  // namespace maplang

#endif  // MAPLANG_HTTP_RESPONSE_WITH_ADDRESS_AS_BODY_H_
