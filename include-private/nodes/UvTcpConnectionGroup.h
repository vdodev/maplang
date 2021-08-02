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

#ifndef __MAPLANG_UVTCPCONNECTIONGROUP_H__
#define __MAPLANG_UVTCPCONNECTIONGROUP_H__

#include <unordered_map>

#include "maplang/IGroup.h"

namespace maplang {

class UvTcpImpl;

class UvTcpConnectionGroup : public IGroup, public IImplementation {
 public:
  UvTcpConnectionGroup();
  ~UvTcpConnectionGroup() override = default;

  size_t getInterfaceCount() override;
  std::string getInterfaceName(size_t nodeIndex) override;

  std::shared_ptr<IImplementation> getInterface(
      const std::string& nodeName) override;

  IPathable* asPathable() override { return nullptr; }
  ISource* asSource() override { return nullptr; }
  IGroup* asGroup() override { return this; }

 private:
  const std::shared_ptr<UvTcpImpl> mImpl;
  std::unordered_map<std::string, std::shared_ptr<IImplementation>> mInterfaces;
};

}  // namespace maplang

#endif  // __MAPLANG_UVTCPCONNECTIONGROUP_H__
