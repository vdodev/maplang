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

#include "HttpResponseWithAddressAsBody.h"
#include "maplang/DataGraph.h"
#include "maplang/NodeFactory.h"
#include "maplang/json.hpp"

using namespace std;
using namespace maplang;
using namespace nlohmann;

static void registerNodes() {
  auto nodeFactory = NodeFactory::defaultFactory();

  nodeFactory->registerNodeFactory(
      "HTTP response with address as body",
      [](const json& initParameters) {
        return make_shared<HttpResponseWithAddressAsBody>();
      });
}

int main(int argc, char** argv) {
  registerNodes();

  DataGraph graph;

  graph.connect("Setup TCP Listener", "initialized", "Start TCP Listen");
  graph.connect("TCP Server", "Data Received", "HTTP Request Extractor");
  graph.connect(
      "HTTP Request Extractor",
      "New Request",
      "Create HTTP Response With Remote Address as Body");
  graph.connect(
      "Create HTTP Response With Remote Address as Body",
      "On Response",
      "HTTP Response Writer");
  graph.connect("HTTP Response Writer", "Http Data", "TCP Send");
  graph.connect(
      "TCP Server",
      "Connection Closed",
      "Remove Connection From HTTP Extractor");

  graph.setNodeInstance("TCP Server", "TCP Server Instance");
  graph.setInstanceInitParameters("TCP Server Instance", R"(
      {
            "disableNaglesAlgorithm": true
      })"_json);
  graph.setInstanceType("TCP Server Instance", "TCP Server");

  graph.setNodeInstance("Start TCP Listen", "Start TCP Listen Instance");
  graph.setInstanceImplementationToGroupInterface(
      "Start TCP Listen Instance",
      "TCP Server Instance",
      "Listener");

  graph.setNodeInstance(
      "HTTP Request Extractor Router",
      "HTTP Request Extractor Router Instance");
  graph.setInstanceInitParameters("HTTP Request Extractor Router Instance", R"(
        {
          "nodeImplementation": "HTTP Request Extractor",
          "key": "TcpConnectionId"
        })"_json);
  graph.setInstanceType("HTTP Request Extractor Router Instance", "Contextual");

  graph.setNodeInstance(
      "HTTP Request Extractor",
      "HTTP Request Extractor Instance");
  graph.setInstanceImplementationToGroupInterface(
      "HTTP Request Extractor Instance",
      "HTTP Request Extractor Router Instance",
      "Context Router");

  graph.setNodeInstance(
      "Create HTTP Response With Remote Address as Body",
      "Create HTTP Response Instance");
  graph.setInstanceType(
      "Create HTTP Response Instance",
      "HTTP response with address as body");

  graph.setNodeInstance(
      "HTTP Response Writer",
      "HTTP Response Writer Instance");
  graph.setInstanceType(
      "HTTP Response Writer Instance",
      "HTTP Response Writer");

  graph.setNodeInstance("TCP Send", "TCP Send Instance");
  graph.setInstanceImplementationToGroupInterface(
      "TCP Send Instance",
      "TCP Server Instance",
      "Sender");

  graph.setNodeInstance(
      "Remove Connection From HTTP Extractor",
      "Remove Connection From HTTP Extractor Instance");
  graph.setInstanceImplementationToGroupInterface(
      "Remove Connection From HTTP Extractor Instance",
      "HTTP Request Extractor Router Instance",
      "Context Remover");

  // The Send-Once Setup TCP Listener comes last becase it kicks of everything
  // and sends a Packet as soon as it is created.
  graph.setNodeInstance("Setup TCP Listener", "Setup TCP Listener Instance");
  graph.setInstanceInitParameters(
      "Setup TCP Listener Instance",
      R"({ "Port": 8080 })"_json);
  graph.setInstanceType("Setup TCP Listener Instance", "Send Once");

  graph.validateConnections();

  while (true) {
    this_thread::sleep_for(std::chrono::seconds(1));
  }

  return 0;
}
