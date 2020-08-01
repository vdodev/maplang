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

#include "ContextualNode.h"
#include "DataGraph.h"
#include "NodeRegistration.h"
#include "json.hpp"
#include "nodes/SendOnce.h"
#include "nodes/tcp/UvTcpServerNode.h"
#include "nodes/VolatileKeyValueStore.h"
#include "UvLoopRunner.h"
#include "nodes/HttpRequestExtractor.h"
#include "nodes/SendOnce.h"
#include "nodes/HttpResponseWriter.h"
#include "nodes/HttpResponseWithAddressAsBody.h"

using namespace std;
using namespace dgraph;
using namespace nlohmann;

static void registerNodes() {
  auto registration = NodeRegistration::defaultRegistration();

  registration->registerPartitionedNodeFactory(
      "TCP Server",
      [](const nlohmann::json& initParameters) {
        return make_shared<UvTcpServerNode>();
      });

  registration->registerNodeFactory(
      "HTTP Request Extractor",
      [](const json& initParameters) {
        const shared_ptr<INode> node = make_shared<HttpRequestExtractor>(initParameters);
        return node;
      });

  registration->registerNodeFactory(
      "Send Once",
      [](const json& initParameters) {
        return make_shared<SendOnce>(initParameters);
      });

  registration->registerNodeFactory(
      "HTTP Response Writer",
      [](const json& initParameters) {
        return make_shared<HttpResponseWriter>();
      });

  registration->registerNodeFactory(
      "HTTP response with address as body",
      [](const json& initParameters) {
        return make_shared<HttpResponseWithAddressAsBody>();
      });
}

int main(int argc, char** argv) {
  auto kvStore = make_shared<VolatileKeyValueStore>(R"(
  {
    "key": "TcpConnectionId",
    "retainBuffers": false
  }
  )"_json);

  registerNodes();

  const auto registration = NodeRegistration::defaultRegistration();
  const shared_ptr<IPartitionedNode> tcpServer =
      registration->createPartitionedNode(
          "TCP Server", R"(
      {
            "disableNaglesAlgorithm": true
      })"_json);


  const auto tcpSend = tcpServer->getPartition("Sender");
  const auto httpRequestExtractor = make_shared<ContextualNode>(
      "HTTP Request Extractor", "TcpConnectionId", nullptr);
  const auto httpResponseWriter =
      registration->createNode("HTTP Response Writer", nullptr);
  const auto httpResponseWithAddressAsBody =
      registration->createNode("HTTP response with address as body", nullptr);
  const auto tcpReceive = tcpServer->getPartition("Receiver");
  const auto tcpListen = tcpServer->getPartition("Listener");
  const auto tcpDisconnector = tcpServer->getPartition("Disconnector");
  const auto setupListen =
      registration->createNode("Send Once", R"({ "Port": 8080 })"_json);

  UvLoopRunner uvLoopRunner;
  DataGraph graph(uvLoopRunner.getLoop());

  graph.connect(httpResponseWriter, "Http Data", tcpSend);
  graph.connect(httpResponseWithAddressAsBody, "On Response",
                httpResponseWriter);
  graph.connect(httpRequestExtractor->getPartition("Context Router"),
                "New Request", httpResponseWithAddressAsBody);
  graph.connect(tcpReceive, "Data Received",
                httpRequestExtractor->getPartition("Context Router"));
  graph.connect(tcpDisconnector, "Ended",
                httpRequestExtractor->getPartition("Context Remover"));
  graph.connect(setupListen, "initialized", tcpListen);

  uvLoopRunner.waitForExit();

  return 0;
}
