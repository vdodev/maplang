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

#ifndef MAPLANG__JSONUTIL_H_
#define MAPLANG__JSONUTIL_H_

#include <string>

#include "json.hpp"

namespace maplang {

template <class T>
T jsonGetOr(const nlohmann::json& j, const std::string& key, T orValue) {
  if (j.contains(key)) {
    return j[key].get<T>();
  } else {
    return orValue;
  }
}

template <class T>
bool jsonTryGet(const nlohmann::json& j, const std::string& key, T* valueOut) {
  if (!j.contains(key)) {
    return false;
  }

  *valueOut = j[key].get<T>();
  return true;
}

#if __cpp_lib_optional
#include <optional>

template <class T>
std::optional<T> jsonGet(const nlohmann::json& j, const std::string& key) {
  if (!j.contains(key)) {
    return std::optional<T>();
  }

  return j[key].get<T>();
}
#endif

}  // namespace maplang

#endif  // MAPLANG__JSONUTIL_H_
