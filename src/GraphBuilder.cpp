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

#include "maplang/GraphBuilder.h"

#include <cgraph.h>

#include <memory>
#include <sstream>

#include "logging.h"

using namespace std;

namespace maplang {

static optional<bool> getOptionalBoolAttribute(
    void* obj,
    char* attributeName,
    string* notSetReason = nullptr) {
  Agsym_t* attributeSym = agattrsym(obj, attributeName);

  if (attributeSym == nullptr) {
    if (notSetReason != nullptr) {
      *notSetReason = "Attribute does not exist";
    }

    return {};
  }

  const string value = agxget(obj, attributeSym);
  if (value == "true") {
    return true;
  } else if (value == "false") {
    return false;
  }

  if (notSetReason != nullptr) {
    *notSetReason =
        string("'") + attributeName
        + "' must be a boolean value (true or false). Actual value: '" + value
        + "'";
  }

  return {};
}

static optional<string> getOptionalStringAttribute(
    void* obj,
    char* attributeName) {
  Agsym_t* attributeSym = agattrsym(obj, attributeName);

  if (attributeSym == nullptr) {
    return {};
  }

  return agxget(obj, attributeSym);
}

static string getStringAttribute(void* obj, char* attributeName) {
  optional<string> attributeValue =
      getOptionalStringAttribute(obj, attributeName);

  if (!attributeValue || attributeValue->empty()) {
    ostringstream errorStream;

    errorStream << "Could not find attribute '" << attributeName << "'";

    throw runtime_error(errorStream.str());
  }

  return *attributeValue;
}

static optional<bool> getOptionalNodeBoolAttribute(
    Agnode_t* node,
    char* attributeName,
    string* notSetReason = nullptr) {
  string innerNotSetReason;
  optional<bool> value =
      getOptionalBoolAttribute(node, attributeName, &innerNotSetReason);

  if (value.has_value()) {
    return *value;
  }

  if (notSetReason != nullptr) {
    *notSetReason = string("Error getting attribute from node '")
                    + agnameof(node) + "': " + innerNotSetReason;
  }

  return {};
}

static bool getEdgeBoolAttribute(
    Agnode_t* fromNode,
    Agedge_t* edge,
    char* attributeName,
    string* notSetReason) {
  string innerNotSetReason;
  optional<bool> value =
      getOptionalBoolAttribute(edge, attributeName, &innerNotSetReason);

  if (value.has_value()) {
    return *value;
  }

  if (notSetReason != nullptr) {
    *notSetReason = string("Error getting attribute from node '")
                    + agnameof(fromNode) + "' to node '" + agnameof(edge->node)
                    + "': " + innerNotSetReason;
  }

  return {};
}

static string getNodeStringAttribute(Agnode_t* node, char* attributeName) {
  try {
    return getStringAttribute(node, attributeName);
  } catch (const exception& e) {
    throw runtime_error(
        string("Error getting attribute from node '") + agnameof(node)
        + "': " + e.what());
  }
}

static string getEdgeStringAttribute(
    Agnode_t* fromNode,
    Agedge_t* edge,
    char* attributeName) {
  try {
    return getStringAttribute(edge, attributeName);
  } catch (const exception& e) {
    throw runtime_error(
        string("Error getting attribute edge connecting from node '")
        + agnameof(fromNode) + "' to node '" + agnameof(edge->node)
        + "': " + e.what());
  }
}

static ostringstream agErrorStream;
static int onAgError(char* msg) {
  agErrorStream << msg << endl;
  return 0;
}

static string readFileIntoString(const string& fileName) {
  FILE* file = fopen(fileName.c_str(), "rb");
  fseek(file, 0, SEEK_END);
  const auto fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  char* fileContents = reinterpret_cast<char*>(malloc(fileSize + 1));
  if (fileContents == nullptr) {
    throw bad_alloc();
  }

  const auto fileContentsDeleter =
      shared_ptr<char>(fileContents, [](char* buffer) { free(buffer); });

  const size_t numberOfBytesRead = fread(fileContents, 1, fileSize, file);
  if (numberOfBytesRead != fileSize) {
    throw runtime_error(string("Error reading file '") + fileName + "'");
  }
  fileContents[fileSize] = 0;

  return fileContents;
}

std::shared_ptr<DataGraph> buildDataGraphFromFile(
    const Factories& factories,
    const string& fileName) {
  const string dataGraphString = readFileIntoString(fileName);

  return buildDataGraph(factories, dataGraphString);
}

shared_ptr<DataGraph> buildDataGraph(
    const Factories& factories,
    const string& dotGraphString) {
  agseterrf(onAgError);

  const auto dataGraph = make_shared<DataGraph>(factories);

  const auto cgraph = shared_ptr<Agraph_t>(
      agmemread(dotGraphString.c_str()),
      [](Agraph_t* cgraph) { agclose(cgraph); });

  if (cgraph == nullptr) {
    ostringstream errorStream;
    errorStream << "Error parsing graph: '" << agErrorStream.str()
                << "' Graph:" << endl
                << dotGraphString << endl;
    agErrorStream.clear();

    string errorMessage = errorStream.str();

    loge("%s", errorMessage.c_str());
    throw runtime_error(errorMessage);
  }

  for (Agraph_t* subgraph = agfstsubg(cgraph.get()); subgraph != nullptr;
       subgraph = agnxtsubg(subgraph)) {
    const string subgraphName = agnameof(subgraph);
    const string instanceName = getStringAttribute(subgraph, "instance");

    const bool allowIncoming =
        getOptionalBoolAttribute(subgraph, "allowIncoming").value_or(false);
    const bool allowOutgoing =
        getOptionalBoolAttribute(subgraph, "allowOutgoing").value_or(false);

    dataGraph->createNode(subgraphName, allowIncoming, allowOutgoing);
    dataGraph->setNodeInstance(subgraphName, instanceName);

    logd(
        "Found subgraph %s, instance %s\n",
        subgraphName.c_str(),
        instanceName.c_str());
  }

  for (Agnode_t* node = agfstnode(cgraph.get()); node;
       node = agnxtnode(cgraph.get(), node)) {
    const std::string nodeName = agnameof(node);

    const string instanceName = getNodeStringAttribute(node, "instance");
    const bool allowIncoming =
        getOptionalNodeBoolAttribute(node, "allowIncoming").value_or(false);
    const bool allowOutgoing =
        getOptionalNodeBoolAttribute(node, "allowOutgoing").value_or(false);

    dataGraph->createNode(nodeName, allowIncoming, allowOutgoing);
    dataGraph->setNodeInstance(nodeName, instanceName);

    const optional<string> initParameters =
        getOptionalStringAttribute(node, "initParameters");
    if (initParameters) {
      dataGraph->setInstanceInitParameters(
          instanceName,
          nlohmann::json::parse(*initParameters));
    }

    logd("Found node %s instance %s\n", nodeName.c_str(), instanceName.c_str());
  }

  for (Agnode_t* fromNode = agfstnode(cgraph.get()); fromNode;
       fromNode = agnxtnode(cgraph.get(), fromNode)) {
    if (AGTYPE(fromNode) != AGNODE) {
      continue;
    }
    const std::string fromNodeName = agnameof(fromNode);

    for (Agedge_t* edge = agfstout(cgraph.get(), fromNode); edge;
         edge = agnxtout(cgraph.get(), edge)) {
      Agnode_t* const toNode = edge->node;
      const string toNodeName = agnameof(toNode);

      const string channel = getEdgeStringAttribute(fromNode, edge, "label");

      if (channel.empty()) {
        throw runtime_error(
            "label (i.e. output channel) cannot be empty in edge from node '"
            + fromNodeName + "' to '" + toNodeName + "'");
      }

      dataGraph->connect(fromNodeName, channel, toNodeName);

      logd(
          "Found connection %s -> %s (channel %s)\n",
          fromNodeName.c_str(),
          toNodeName.c_str(),
          channel.c_str());
    }
  }

  return dataGraph;
}

void implementDataGraphFromFile(
    const shared_ptr<DataGraph>& dataGraph,
    const string& fileName) {
  const string implementationString = readFileIntoString(fileName);
  implementDataGraph(dataGraph, implementationString);
}

static nlohmann::json getJson(
    const nlohmann::json& containingObject,
    const string& containingKey,
    const string& key) {
  if (!containingObject.contains(key)) {
    ostringstream errorStream;
    errorStream << "'" << key << "' is missing in '" << containingKey << "'";
    throw runtime_error(errorStream.str());
  }

  return containingObject[key];
}

static string getNonEmptyStringOrThrow(
    const nlohmann::json& containingObject,
    const string& containingKey,
    const string& key) {
  const nlohmann::json& value = getJson(containingObject, containingKey, key);

  if (!value.is_string()) {
    ostringstream errorStream;
    errorStream << "'" << key << "' must be a string in '" << containingKey
                << "'. Actual type: " << value.type_name();

    throw runtime_error(errorStream.str());
  }

  string stringValue = value.get<string>();
  if (stringValue.empty()) {
    ostringstream errorStream;
    errorStream << "'" << key << "' cannot be an empty string in '"
                << containingKey << "'";

    throw runtime_error(errorStream.str());
  }

  return stringValue;
}

static nlohmann::json getObjectOrThrow(
    const nlohmann::json& containingObject,
    const string& containingKey,
    const string& key) {
  const nlohmann::json& value = getJson(containingObject, containingKey, key);

  if (!value.is_object()) {
    ostringstream errorStream;
    errorStream << "'" << key << "' must be an object in '" << containingKey
                << "'. Actual type: " << value.type_name();

    throw runtime_error(errorStream.str());
  }

  return value;
}

void implementDataGraph(
    const shared_ptr<DataGraph>& dataGraph,
    const string& implementationJson) {
  nlohmann::json implementation =
      nlohmann::json::parse(implementationJson.c_str());

  if (!implementation.is_object()) {
    throw runtime_error(
        string("Root object in implementation JSON must be an object. Actual "
               "type: ")
        + implementation.type_name());
  }

  for (auto& [instanceName, instanceImplementation] : implementation.items()) {
    logd("Implementing instance '%s'\n", instanceName.c_str());

    // Set initParameters

    if (instanceImplementation.contains("initParameters")) {
      if (!instanceImplementation["initParameters"].is_object()) {
        throw runtime_error(
            "initParameters must be an object for instance '" + instanceName
            + "'");
      }

      dataGraph->insertInstanceInitParameters(
          instanceName,
          instanceImplementation["initParameters"]);
    }

    // Set Implementation
    if (!instanceImplementation.contains("type")
        && !instanceImplementation.contains("implementationFromGroup")) {
      throw runtime_error(
          "Instance '" + instanceName
          + "' must contain either 'type' or 'implementationFromGroup'");
    }

    if (instanceImplementation.contains("type")) {
      const string implementingType = getNonEmptyStringOrThrow(
          instanceImplementation,
          instanceName,
          "type");

      dataGraph->setInstanceType(instanceName, implementingType);
    } else {
      const auto implementationFromGroup = getObjectOrThrow(
          instanceImplementation,
          instanceName,
          "implementationFromGroup");

      const nlohmann::json groupInstance = getNonEmptyStringOrThrow(
          implementationFromGroup,
          instanceName + ".implementationFromGroup",
          "groupInstance");
      const nlohmann::json groupInterface = getNonEmptyStringOrThrow(
          implementationFromGroup,
          instanceName + ".implementationFromGroup",
          "groupInterface");

      dataGraph->setInstanceImplementationToGroupInterface(
          instanceName,
          groupInstance,
          groupInterface);
    }

    // Map subgraph instances to the group's interfaces
    if (instanceImplementation.contains("instanceToInterfaceMap")) {
      const nlohmann::json& instanceToInterfaceMap = getObjectOrThrow(
          instanceImplementation,
          instanceName,
          "instanceToInterfaceMap");
      for (auto& [interfaceInstanceName, _] : instanceToInterfaceMap.items()) {
        const nlohmann::json& interfaceImplementation = getObjectOrThrow(
            instanceToInterfaceMap,
            instanceName + ".instanceToInterfaceMap",
            interfaceInstanceName);

        const string& nameOfGroupInterface = getNonEmptyStringOrThrow(
            interfaceImplementation,
            instanceName + ".instanceToInterfaceMap." + interfaceInstanceName,
            "interface");

        dataGraph->setInstanceImplementationToGroupInterface(
            interfaceInstanceName,
            instanceName,
            nameOfGroupInterface);
      }
    }
  }
}

}  // namespace maplang
