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

#ifndef MAPLANG_INCLUDE_MAPLANG_PARAMETERROUTER_H_
#define MAPLANG_INCLUDE_MAPLANG_PARAMETERROUTER_H_

#include "maplang/Factories.h"
#include "maplang/IImplementation.h"

namespace maplang {

class ParameterRouter final : public IPathable, public IImplementation {
 public:
  static const std::string kInitParameter_RoutingKey;

 public:
  ParameterRouter(
      const Factories& factories,
      const nlohmann::json& initParameters);
  ~ParameterRouter() override = default;

  void handlePacket(const PathablePacket& packet) override;

  ISource* asSource() override { return nullptr; }
  IPathable* asPathable() override { return this; }
  IGroup* asGroup() override { return nullptr; }

 private:
  const Factories mFactories;
  const nlohmann::json mInitParameters;

  nlohmann::json_pointer<nlohmann::basic_json<>> mRoutingKey;
};

}  // namespace maplang

#endif  // MAPLANG_INCLUDE_MAPLANG_PARAMETERROUTER_H_
