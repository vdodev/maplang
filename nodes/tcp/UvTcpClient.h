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

#ifndef __MAPLANG_UVTCPCLIENT_H__
#define __MAPLANG_UVTCPCLIENT_H__

#include "../../ICohesiveNode.h"
#include "../../ISink.h"
#include "../../ISource.h"
#include "UvTcpConnector.h"
#include "UvTcpReceiver.h"
#include "UvTcpSender.h"

namespace maplang {

class UvTcpClient : public ICohesiveNode {
 public:
  ~UvTcpClient() override = default;

  size_t getNodeCount() override;
  std::string getNodeName(size_t partitionIndex) override;

  std::shared_ptr<INode> getNode(
      const std::string& partitionName) override;

 private:
  const std::shared_ptr<UvTcpSender> mTcpSender;
  const std::shared_ptr<UvTcpReceiver> mTcpReceiver;
  const std::shared_ptr<UvTcpConnector> mTcpConnector;
};

}  // namespace maplang

#endif  // __MAPLANG_UVTCPCLIENT_H__
