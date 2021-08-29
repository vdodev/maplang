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

#ifndef MAPLANG_SRC_NODES_PARAMETEREXTRACTOR_H_
#define MAPLANG_SRC_NODES_PARAMETEREXTRACTOR_H_

#include "maplang/Factories.h"
#include "maplang/IImplementation.h"

namespace maplang {

class ParameterExtractor : public IPathable, public IImplementation {
 public:
  static const std::string kChannel_ExtractedParameter;

 public:
  ParameterExtractor(
      const Factories& factories,
      const nlohmann::json& initParameters);
  ~ParameterExtractor() override = default;

  void handlePacket(const PathablePacket& packet) override;

  ISource* asSource() override { return nullptr; }
  IPathable* asPathable() override { return this; }
  IGroup* asGroup() override { return nullptr; }

 private:
  const Factories mFactories;
  const nlohmann::json mInitParameters;
  const nlohmann::json_pointer<nlohmann::basic_json<>>
      mParameterJsonPointerToExtract;
};

}  // namespace maplang

#endif  // MAPLANG_SRC_NODES_PARAMETEREXTRACTOR_H_
