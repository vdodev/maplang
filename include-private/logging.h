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

#ifndef __MAPLANG_LOGGING_H__
#define __MAPLANG_LOGGING_H__

#include <stdio.h>

#include <cstdarg>

namespace maplang {

#ifndef NDEBUG
__attribute__((__format__(__printf__, 1, 2))) inline void logd(
    const char* fmt,
    ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
}
#else
__attribute__((__format__(__printf__, 1, 2))) inline void logd(
    const char* fmt,
    ...) {}
#endif

__attribute__((__format__(__printf__, 1, 2))) inline void logi(
    const char* fmt,
    ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
}

__attribute__((__format__(__printf__, 1, 2))) inline void logw(
    const char* fmt,
    ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  printf("\n");
  va_end(args);
}

__attribute__((__format__(__printf__, 1, 2))) inline void loge(
    const char* fmt,
    ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  printf("\n");
  va_end(args);
}
}  // namespace maplang

#endif  // __MAPLANG_LOGGING_H__
