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

#include "maplang/DotExporter.h"

#include <sstream>

namespace maplang {

std::string DotExporter::ExportGraph(
    DataGraph* graph,
    const std::string& graphName) {
  std::vector<std::shared_ptr<GraphNode>> elements;

  std::ostringstream out;
  std::unordered_map<std::shared_ptr<GraphNode>, std::string> nodeToNameMap;

  size_t nodeIndex = 0;
  graph->visitNodes([&nodeToNameMap, &nodeIndex, &out](
                        const std::shared_ptr<GraphNode>& element) {
    nodeToNameMap[element] = element->name;
    nodeIndex++;

    // out << "    <node id=\"" << elementName << "\"/>\n";
  });

  // out << std::endl;

  out << "strict digraph " << graphName << " {" << std::endl;
  for (const auto& elementNamePair : nodeToNameMap) {
    const std::shared_ptr<GraphNode>& element = elementNamePair.first;
    const std::string& name = elementNamePair.second;

    for (const auto& edgePair : element->forwardEdges) {
      const std::string& channel = edgePair.first;
      const std::vector<GraphEdge>& channelEdges = edgePair.second;

      for (const auto& edge : channelEdges) {
        const auto otherElementName = nodeToNameMap[edge.next];

        out << "    \"" << name << "\" -> \"" << otherElementName << "\"";

        if (!edge.channel.empty()) {
          out << " [label=\"" << edge.channel << "\"]";
        }

        out << std::endl;
      }
    }
  }

  out << "}" << std::endl;

  return out.str();
}

}  // namespace maplang
