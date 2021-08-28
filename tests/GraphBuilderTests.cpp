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

#include <thread>

#include "gtest/gtest.h"
#include "maplang/DataGraph.h"
#include "maplang/FactoriesBuilder.h"
#include "maplang/GraphBuilder.h"
#include "maplang/LambdaPathable.h"

using namespace std;

namespace maplang {

class GraphBuilderTests : public testing::Test {
 public:
  GraphBuilderTests() : mFactories(FactoriesBuilder().BuildFactories()) {}

  const std::shared_ptr<const IFactories> mFactories;
};

TEST_F(GraphBuilderTests, WhenASimplGraphIsLoaded_ItHasTheCorrectConnections) {
  const string dotGraph = R"(
    digraph SomeGraphName {
      "Node 1" [instance="Node 1 instance", allowOutgoing=true]
      "Node 2" [instance="Node 2 instance", allowIncoming=true, allowOutgoing=true]
      "Node 3" [instance="Node 3 instance", allowIncoming=true]

      "Node 1" -> "Node 2" [label="On Node 1 Output"]
      "Node 1" -> "Node 3" [label="On Node 1 Output"]
      "Node 2" -> "Node 3" [label="On Node 2 Output"]
    }
  )";
  const auto dataGraph = buildDataGraph(mFactories, dotGraph);

  bool foundNode1 = false;
  bool foundNode2 = false;
  bool foundNode3 = false;
  bool foundN1ToN2Connection = false;
  bool foundN1ToN3Connection = false;
  bool foundN2ToN3Connection = false;
  dataGraph->visitNodes(
      [&foundNode1,
       &foundNode2,
       &foundNode3,
       &foundN1ToN2Connection,
       &foundN1ToN3Connection,
       &foundN2ToN3Connection](const shared_ptr<GraphNode>& node) {
        if (node->name == "Node 1") {
          foundNode1 = true;

          for (auto& nameEdgePair : node->forwardEdges) {
            const auto& channel = nameEdgePair.first;
            const auto& edgesForChannel = nameEdgePair.second;

            if (channel != "On Node 1 Output") {
              GTEST_FAIL() << "Unexpected channel '" << channel
                           << "' for node 'Node 1'";
            }

            for (const auto& edge : edgesForChannel) {
              if (edge.next->name == "Node 2") {
                foundN1ToN2Connection = true;
              } else if (edge.next->name == "Node 3") {
                foundN1ToN3Connection = true;
              } else {
                GTEST_FAIL() << "Node 1 connected to unexpected next node '"
                             << edge.next->name << "'";
              }
            }
          }
        } else if (node->name == "Node 2") {
          foundNode2 = true;

          for (auto& nameEdgePair : node->forwardEdges) {
            const auto& channel = nameEdgePair.first;
            const auto& edgesForChannel = nameEdgePair.second;

            if (channel != "On Node 2 Output") {
              GTEST_FAIL() << "Unexpected channel '" << channel
                           << "' for node 'Node 2'";
            }

            for (const auto& edge : edgesForChannel) {
              if (edge.next->name == "Node 3") {
                foundN2ToN3Connection = true;
              } else {
                GTEST_FAIL() << "Node 2 connected to unexpected next node '"
                             << edge.next->name << "'";
              }
            }
          }
        } else if (node->name == "Node 3") {
          foundNode3 = true;

          for (auto& nameEdgePair : node->forwardEdges) {
            GTEST_FAIL() << "Node 3 has an unexpected outgoing connection";
          }
        } else {
          GTEST_FAIL() << "Unexpected node '" << node->name << "'";
        }
      });

  ASSERT_TRUE(foundNode1);
  ASSERT_TRUE(foundNode2);
  ASSERT_TRUE(foundNode3);
  ASSERT_TRUE(foundN1ToN2Connection);
  ASSERT_TRUE(foundN1ToN3Connection);
  ASSERT_TRUE(foundN2ToN3Connection);
}

}  // namespace maplang
