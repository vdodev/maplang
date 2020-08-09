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

#include "BlueprintBuilder.h"
#include <unordered_map>
#include "json.hpp"
#include "graph/Graph.h"

using namespace std;
using namespace nlohmann;

namespace maplang {

static const string kKey_Graphs = "graphs";
static const string kKey_NodeTypes = "nodeTypes";
static const string kKey_NodeInstances = "nodeInstances";
static const string kKey_TypeImplementations = "typeImplementations";
static const string kKey_NodeImplementations = "nodeImplementations";
static const string kKey_CohesiveGroupImplementations = "cohesiveGroupImplementations";
static const string kKey_Connections = "connections";

void BlueprintBuilder::build(DataGraph *graph, const nlohmann::json &blueprint) {
  const string requiredKeys[] = {
      kKey_Graphs,
      kKey_NodeTypes,
      kKey_NodeInstances,
      kKey_Connections,
  };

  for (const auto& requiredKey : requiredKeys) {
    if (!blueprint.contains(requiredKey)) {
      throw runtime_error("'" + requiredKey + "' element is missing from blueprint.");
    }
  }

  if (!blueprint.contains(kKey_TypeImplementations) && !blueprint.contains(kKey_NodeImplementations)) {
    throw runtime_error("'" + kKey_TypeImplementations + "' or '" + kKey_NodeImplementations + "' element is required in blueprint.");
  }

  const auto& typeConfig = blueprint[kKey_NodeTypes];
  const auto& instanceConfig = blueprint[kKey_NodeInstances];

  // Build a graph of the connections, find the tails, work backward to find the heads, then add to the graph in stack-order.
  for (auto it = instanceConfig.cbegin(); it != instanceConfig.end(); it++) {
    const string& instanceName = it.key();
    const json& instanceInfo = it.value();

    Graph<string> graph;
  }
}

}  // namespace maplang
