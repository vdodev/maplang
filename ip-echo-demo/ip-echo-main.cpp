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
#include "maplang/GraphBuilder.h"
#include "maplang/ImplementationFactory.h"
#include "maplang/json.hpp"

using namespace std;
using namespace maplang;
using namespace nlohmann;

static void registerNodes() {
  auto nodeFactory = ImplementationFactory::defaultFactory();

  nodeFactory->registerFactory(
      "HTTP Response With Remote Address As Body",
      [](const json& initParameters) {
        return make_shared<HttpResponseWithAddressAsBody>();
      });
}

int main(int argc, char** argv) {
  registerNodes();

  const auto graph = buildDataGraphFromFile("../ip-echo-demo/ip-echo-architecture.dot");
  implementDataGraphFromFile(graph, "../ip-echo-demo/ip-echo-implementation.json");

  graph->startGraph();

  while (true) {
    this_thread::sleep_for(std::chrono::seconds(1));
  }

  return 0;
}
