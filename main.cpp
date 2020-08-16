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

#include "maplang/DataGraph.h"
#include "maplang/NodeRegistration.h"
#include "maplang/json.hpp"
#include "nodes/SendOnce.h"
#include "nodes/tcp/UvTcpServerNode.h"
#include "nodes/VolatileKeyValueStore.h"
#include "maplang/UvLoopRunner.h"
#include "nodes/HttpRequestExtractor.h"
#include "nodes/SendOnce.h"
#include "nodes/HttpResponseWriter.h"
#include "nodes/HttpResponseWithAddressAsBody.h"

using namespace std;
using namespace maplang;
using namespace nlohmann;

static void registerNodes() {
  auto registration = NodeRegistration::defaultRegistration();

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
  const shared_ptr<ICohesiveGroup> tcpServer =
      registration->createCohesiveGroup(
          "TCP Server", R"(
      {
            "disableNaglesAlgorithm": true
      })"_json);


  const auto tcpSend = tcpServer->getNode("Sender");
  const auto httpRequestExtractor = registration->createCohesiveGroup(
      "Contextual Node",
      R"(
        {
          "nodeImplementation": "HTTP Request Extractor",
          "key": "TcpConnectionId"
        })"_json);
  const auto httpResponseWriter =
      registration->createNode("HTTP Response Writer", nullptr);
  const auto httpResponseWithAddressAsBody =
      registration->createNode("HTTP response with address as body", nullptr);
  const auto tcpReceive = tcpServer->getNode("Receiver");
  const auto tcpListen = tcpServer->getNode("Listener");
  const auto tcpDisconnector = tcpServer->getNode("Disconnector");
  const auto setupListen =
      registration->createNode("Send Once", R"({ "Port": 8080 })"_json);

  UvLoopRunner uvLoopRunner;
  DataGraph graph(uvLoopRunner.getLoop());

  graph.connect(httpResponseWriter, "Http Data", tcpSend);
  graph.connect(httpResponseWithAddressAsBody, "On Response",
                httpResponseWriter);
  graph.connect(httpRequestExtractor->getNode("HTTP Request Extractor"),
                "New Request", httpResponseWithAddressAsBody);
  graph.connect(tcpReceive, "Data Received",
                httpRequestExtractor->getNode("HTTP Request Extractor"));
  graph.connect(tcpDisconnector, "Ended",
                httpRequestExtractor->getNode("Context Remover"));
  graph.connect(setupListen, "initialized", tcpListen);

  uvLoopRunner.waitForExit();

  return 0;
}
