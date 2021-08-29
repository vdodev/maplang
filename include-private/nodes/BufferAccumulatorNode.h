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

#ifndef MAPLANG_INCLUDE_PRIVATE_NODES_BUFFERACCUMULATOR_H_
#define MAPLANG_INCLUDE_PRIVATE_NODES_BUFFERACCUMULATOR_H_

#include <unordered_map>

#include "maplang/IBufferFactory.h"
#include "maplang/Factories.h"
#include "maplang/IGroup.h"
#include "maplang/IImplementation.h"

namespace maplang {

class BufferAccumulatorNode : public IImplementation, public IGroup {
 public:
  static const std::string kChannel_AccumulatedBuffersReady;

  static const std::string kNodeName_AppendBuffers;
  static const std::string kNodeName_SendAccumulatedBuffers;
  static const std::string kNodeName_ClearBuffers;

 public:
  BufferAccumulatorNode(
      const Factories& factories,
      const nlohmann::json& initData);
  ~BufferAccumulatorNode() override = default;

  size_t getInterfaceCount() override;
  virtual std::string getInterfaceName(size_t interfaceIndex) override;

  virtual std::shared_ptr<IImplementation> getInterface(
      const std::string& interfaceName) override;

  IPathable* asPathable() override { return nullptr; }
  ISource* asSource() override { return nullptr; }
  IGroup* asGroup() override { return this; }

 private:
  const Factories mFactories;
  std::unordered_map<std::string, std::shared_ptr<IImplementation>> mInterfaces;
};

}  // namespace maplang

#endif  // MAPLANG_INCLUDE_PRIVATE_NODES_BUFFERACCUMULATOR_H_
