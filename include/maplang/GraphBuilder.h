/*
 * Copyright 2021 VDO Dev Inc <support@maplang.com>
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

#ifndef MAPLANG_SRC_GRAPHBUILDER_H_
#define MAPLANG_SRC_GRAPHBUILDER_H_

#include "maplang/DataGraph.h"

namespace maplang {

std::shared_ptr<DataGraph> buildDataGraphFromFile(const std::string& fileName);
std::shared_ptr<DataGraph> buildDataGraph(const std::string& dotGraphString);

void implementDataGraphFromFile(
    const std::shared_ptr<DataGraph>& dataGraph,
    const std::string& fileName);

void implementDataGraph(
    const std::shared_ptr<DataGraph>& dataGraph,
    const std::string& implementationJson);

}  // namespace maplang

#endif  // MAPLANG_SRC_GRAPHBUILDER_H_
