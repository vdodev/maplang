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

#ifndef MAPLANG_VOLATILE_KEY_VALUE_SET_H_
#define MAPLANG_VOLATILE_KEY_VALUE_SET_H_

#include "maplang/Factories.h"
#include "maplang/IGroup.h"

namespace maplang {

class VolatileKeyValueSet : public IGroup, public IImplementation {
 public:
  VolatileKeyValueSet(
      const Factories& factories,
      const nlohmann::json& initParameters);
  ~VolatileKeyValueSet() override = default;

  size_t getInterfaceCount() override;
  std::string getInterfaceName(size_t partitionIndex) override;

  std::shared_ptr<IImplementation> getInterface(
      const std::string& partitionName) override;

  IPathable* asPathable() override { return nullptr; }
  ISource* asSource() override { return nullptr; }
  IGroup* asGroup() override { return this; }

 private:
  struct Partition {
    std::string name;
    std::shared_ptr<IImplementation> node;
  };

  std::unordered_map<std::string, Partition> mPartitions;
};

}  // namespace maplang

#endif  // MAPLANG_VOLATILE_KEY_VALUE_SET_H_
