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
#include "maplang/UvLoopRunner.h"
#include "PrintBufferAsString.h"

using namespace std;
using namespace maplang;
using namespace nlohmann;

static void registerNodes() {
  auto registration = NodeRegistration::defaultRegistration();

  registration->registerNodeFactory(
      "Print buffer as string",
      [](const json& initParameters) {
        return make_shared<PrintBufferAsString>();
      });
}

int main(int argc, char** argv) {
  registerNodes();

  const auto registration = NodeRegistration::defaultRegistration();
  const shared_ptr<INode> tcpServer =
      registration->createNode(
          "TCP Connection", R"(
      {
            "disableNaglesAlgorithm": true
      })"_json);

  const auto tcpSender = tcpServer->asGroup()->getNode("Sender");
  const auto orderedPacketSender = registration->createNode("Ordered Packet Sender", nullptr);
  const auto httpResponseExtractor = registration->createNode(
      "Contextual Node",
      R"(
        {
          "nodeImplementation": "HTTP Response Extractor",
          "key": "TcpConnectionId"
        })"_json);
  const auto httpRequestHeaderWriter =
      registration->createNode("HTTP Request Header Writer", nullptr);
  const auto tcpReceiver = tcpServer->asGroup()->getNode("Receiver");
  const auto tcpConnector = tcpServer->asGroup()->getNode("Connector");
  const auto tcpDisconnector = tcpServer->asGroup()->getNode("Disconnector");
  const auto tcpShutdownSender = tcpServer->asGroup()->getNode("Shutdown Sender");

  json connectInfo;
  connectInfo["Address"] = argv[1];
  connectInfo["Port"] = 80;
  const auto setupConnector =
      registration->createNode("Send Once", connectInfo);

  UvLoopRunner uvLoopRunner;
  DataGraph graph(uvLoopRunner.getLoop());

  graph.connect(orderedPacketSender, "First", tcpSender);
  graph.connect(orderedPacketSender, "Last", tcpShutdownSender);
  graph.connect(httpRequestHeaderWriter, "On Request Header Buffer", orderedPacketSender);
  graph.connect(createHttpGetRequest, "On GET Request", httpRequestHeaderWriter);
  graph.connect(tcpConnector, "Connected", createHttpGetRequest);
  graph.connect(tcpConnector, "error", printErrorToConsole);

  graph.connect(httpResponseExtractor, "On Body Data", printBufferAsString);
  graph.connect(httpResponseExtractor, "On Headers", httpStatusRouter);
  graph.connect(tcpReceiver, "On Data", httpResponseExtractor);

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
