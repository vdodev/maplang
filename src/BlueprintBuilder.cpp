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

#include "maplang/BlueprintBuilder.h"

#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "logging.h"
#include "maplang/ICohesiveGroup.h"
#include "maplang/graph/Graph.h"
#include "maplang/json.hpp"

using namespace std;
using namespace nlohmann;

namespace std {
template <>
struct hash<maplang::GraphElement<string>> {
  std::size_t operator()(const maplang::GraphElement<string>& element) const { return hash<string>()(element.item); }
};
}  // namespace std

namespace maplang {

static const string kKey_Graphs = "graphs";
static const string kKey_NodeTypes = "nodeTypes";
static const string kKey_NodeInstances = "nodeInstances";
static const string kKey_TypeImplementations = "typeImplementations";
static const string kKey_NodeImplementations = "nodeImplementations";
static const string kKey_CohesiveGroupImplementations = "cohesiveGroupImplementations";
static const string kKey_Connections = "connections";

static const string kConnectionKey_FromInstance = "fromInstance";
static const string kConnectionKey_FromChannel = "fromChannel";
static const string kConnectionKey_FromPathable = "fromPathable";
static const string kConnectionKey_ToInstance = "toInstance";
static const string kConnectionKey_ToPathable = "toPathable";

static const string kNodeInstanceKey_Type = "type";
static const string kNodeInstanceKey_CohesiveGroup = "cohesiveGroup";

static void
requireKeys(const vector<string>& requiredKeys, const json& inParameters, const std::string niceParameterObjectName) {
  for (const auto& requiredKey : requiredKeys) {
    if (!inParameters.contains(requiredKey)) {
      throw runtime_error("'" + requiredKey + "' element is missing from " + niceParameterObjectName + ".");
    }
  }
}

void BlueprintBuilder::build(DataGraph* graph, const nlohmann::json& blueprint) {
  requireKeys(
      {
          kKey_Graphs,
          kKey_NodeTypes,
          kKey_NodeInstances,
          kKey_Connections,
      },
      blueprint,
      "blueprint");

  if (!blueprint.contains(kKey_TypeImplementations) && !blueprint.contains(kKey_NodeImplementations)) {
    throw runtime_error(
        "'" + kKey_TypeImplementations + "' or '" + kKey_NodeImplementations + "' element is required in blueprint.");
  }

  const auto& typeConfig = blueprint[kKey_NodeTypes];
  const auto& instanceConfig = blueprint[kKey_NodeInstances];
  const auto& connectionsConfig = blueprint[kKey_Connections];

  // Build a graph of the connections, find the tails, work backward to find the heads, then add to the graph in
  // stack-order.
  Graph<string> instanceGraph;
  for (auto it = connectionsConfig.cbegin(); it != connectionsConfig.end(); it++) {
    const json& connection = it.value();

    requireKeys(
        {kConnectionKey_FromInstance, kConnectionKey_FromChannel, kConnectionKey_ToInstance},
        connection,
        "connections[" + it.key() + "]");

    const string& fromInstance = connection[kConnectionKey_FromInstance].get<string>();
    const string& fromChannel = connection[kConnectionKey_FromChannel].get<string>();
    const string& toInstance = connection[kConnectionKey_ToInstance].get<string>();

    string fromPathableId;
    string toPathableId;

    if (connection.contains(kConnectionKey_FromPathable)) {
      fromPathableId = connection[kConnectionKey_FromPathable].get<string>();
    }

    if (connection.contains(kConnectionKey_ToPathable)) {
      toPathableId = connection[kConnectionKey_ToPathable].get<string>();
    }

    instanceGraph.connect(fromInstance, fromChannel, toInstance, fromPathableId, toPathableId);
  }

  queue<shared_ptr<GraphElement<string>>> toProcess;
  instanceGraph.visitGraphElements([&toProcess](const shared_ptr<GraphElement<string>>& graphElement) {
    if (graphElement->forwardEdges.empty()) {
      toProcess.push(graphElement);
    }
  });

  struct ConnectionInfo {
    string fromInstance;
    string fromChannel;
    string toInstance;
    string fromPathableId;
    string toPathableId;
  };

  // TODO: Check for duplicates (types, CGs, instances, impls).

  bool anyError = false;
  unordered_set<shared_ptr<GraphElement<string>>> alreadyProcessed;
  queue<ConnectionInfo> connectInOrder;
  unordered_map<string, shared_ptr<INode>> nodes;                    // Instance ID => implementation
  unordered_map<string, shared_ptr<ICohesiveGroup>> cohesiveGroups;  // Instance ID => implementation

  // Create CohesiveGroup instances
  instanceGraph.visitGraphElementsHeadsLast(
      [&cohesiveGroups, &instanceConfig](const shared_ptr<GraphElement<string>>& graphElement) {
        // Get the instance info from instanceConfig
        // Find its implementation - instance or type impl
        // Instantiate
        // Add to cohesiveGroups
      });

  // Create Node Instances
  instanceGraph.visitGraphElementsHeadsLast([&nodes](const shared_ptr<GraphElement<string>>& graphElement) {

  });

  // Connect

  while (!toProcess.empty()) {
    const shared_ptr<GraphElement<string>> toElement = toProcess.front();
    toProcess.pop();

    const string& toInstanceName = toElement->item;

    for (const auto& weakGraphElement : toElement->backEdges) {
      const auto fromElement = weakGraphElement.lock();
      const bool alreadyProcessedElement = alreadyProcessed.find(fromElement) != alreadyProcessed.end();
      if (alreadyProcessedElement) {
        continue;
      }

      const auto& fromInstanceName = fromElement->item;
      const auto& fromPathableId = fromElement->pathableId;

      if (!instanceConfig.contains(fromInstanceName)) {
        anyError = true;
        loge("Cannot connect node %s. No such node specified in nodeInstances.\n", fromInstanceName.c_str());
        continue;
      }

      const json& fromInstance = instanceConfig[fromInstanceName];

      toProcess.push(fromElement);
      alreadyProcessed.insert(fromElement);

      // Look through the edges and find all that connect the from and to nodes (may have different pathables).
      for (const auto& edgePair : fromElement->forwardEdges) {
        const auto& edgeKey = edgePair.first;
        const auto& edgeToElement = edgePair.second;

        if (edgeKey.toGraphElement == toElement) {
          const auto& fromChannel = edgeKey.channel;
          const auto& toInstanceName = toElement->item;
          const auto& toPathableId = toElement->pathableId;

          if (!instanceConfig.contains(toInstanceName)) {
            anyError = true;
            loge("Cannot connect node %s. No such node specified in nodeInstances.\n", toInstanceName.c_str());
            continue;
          }

          const json& toNodeInstance = instanceConfig[toInstanceName];
          if (!toNodeInstance.contains(kNodeInstanceKey_Type)) {
            anyError = true;
            loge("Node instance '%s' must contain 'type'.\n", toInstanceName.c_str());
          }
          // Get impl - check node impls first, then type impls. Error if none.
          // Create implementation
        }
      }
    }
  }

  while (!connectInOrder.empty()) {
    ConnectionInfo connectionInfo = move(connectInOrder.front());
    connectInOrder.pop();
  }
}

}  // namespace maplang
