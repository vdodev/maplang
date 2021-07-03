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

#include <maplang/LambdaSink.h>

#include <thread>

#include "gtest/gtest.h"
#include "maplang/DataGraph.h"
#include "maplang/DotExporter.h"

using namespace std;

namespace maplang {

TEST(
    WhenSomeNodesAreConnected,
    TheyAppearInDotML) {
  DataGraph graph;

  graph.connect("Node 1", "onNode1Output", "Node 2");
  graph.connect("Node 1", "onNode1ProducedSomethingElse", "Node 3");
  graph.connect("Node 2", "onNode2Output", "Node 3");

  std::string dot = DotExporter::ExportGraph(&graph, "TestGraph");

  cout << dot << endl;

  ASSERT_NE(string::npos, dot.find("element0"));
  ASSERT_NE(string::npos, dot.find("element1"));
  ASSERT_NE(string::npos, dot.find("element2"));

  ASSERT_NE(string::npos, dot.find("onNode1Output"));
  ASSERT_NE(string::npos, dot.find("onNode1ProducedSomethingElse"));
  ASSERT_NE(string::npos, dot.find("onNode2Output"));

  ASSERT_EQ(string::npos, dot.find("element3"));
}

}  // namespace maplang