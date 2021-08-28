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

#ifndef __MAPLANG_BUFFER_H__
#define __MAPLANG_BUFFER_H__

#include <stddef.h>
#include <stdint.h>

#include <memory>

namespace maplang {

struct Buffer {
  Buffer() : data(nullptr), length(0) {}
  Buffer(std::shared_ptr<uint8_t[]> data, size_t length)
      : data(data), length(length) {}
  Buffer(const std::string& str) {
    data = std::shared_ptr<uint8_t[]>(new uint8_t[str.length()]);
    length = str.length();

    str.copy(reinterpret_cast<char*>(data.get()), str.length());
  }

  std::shared_ptr<uint8_t[]> data;
  size_t length;
};

}  // namespace maplang
#endif  // __MAPLANG_BUFFER_H__
