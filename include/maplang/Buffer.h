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

#ifndef MAPLANG_BUFFER_H_
#define MAPLANG_BUFFER_H_

#include <climits>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace maplang {

struct Buffer {
  Buffer();
  Buffer(const std::shared_ptr<uint8_t>& data, size_t length);
  explicit Buffer(const std::string& str);

  Buffer slice(size_t offset, size_t length = SIZE_MAX) const;

  std::shared_ptr<uint8_t> data;
  size_t length;
};

}  // namespace maplang
#endif  // MAPLANG_BUFFER_H_
