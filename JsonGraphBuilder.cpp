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
#include "JsonGraphBuilder.h"

#include "json.hpp"

using namespace std;
using namespace nlohmann;

namespace dgraph {}  // namespace dgraph

/*
 * Config format
 * array of:
 * {
 *      nodeInstanceName: string,
 *      nodeType: string, // The name the node is registered under
 *      parameters: {
 *          // Type-specific
 *      }
 *      next: [
 *          { nodeInstanceName: string, channelName: string, subChannel: number
 * },
 *          ...
 *      ]
 * }
 */
/*
const std::shared_ptr<DataGraph> JsonGraphBuilder::buildGraph(const std::string
&config) const { auto data = json::parse(config);

}
*/
