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

#include <maplang/INode.h>

namespace maplang {

class ParameterExtractor : public IPathable, public INode {
 public:
  static const std::string kChannel_ExtractedParameter;

 public:
  ParameterExtractor(const nlohmann::json& initParameters);
  ~ParameterExtractor() override = default;

  void handlePacket(const PathablePacket& packet) override;

  ISink* asSink() override { return nullptr; }
  ISource* asSource() override { return nullptr; }
  IPathable* asPathable() override { return this; }
  ICohesiveGroup* asGroup() override { return nullptr; }

 private:
  const std::string mParameterNameToExtract;
};

}  // namespace maplang

#endif  // MAPLANG_SRC_NODES_PARAMETEREXTRACTOR_H_
