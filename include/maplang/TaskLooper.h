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

#ifndef __MAPLANG_TASKLOOPER_H_
#define __MAPLANG_TASKLOOPER_H_

#include <maplang/UvLoopRunner.h>

#include <atomic>
#include <functional>
#include <thread>

class TaskLooper final {
 public:
  TaskLooper(
      const std::shared_ptr<uv_loop_t>& uvLoop,
      std::function<void()>&& task);

  ~TaskLooper();

  void start();
  void requestStop();

 private:
  const std::function<void()> mTask;
  const std::shared_ptr<uv_loop_t> mUvLoop;
  uv_idle_t mUvIdle;
  bool mStarted;

 private:
  static void loopWrapper(uv_idle_t* handle);
  void loop();
};

#endif  // __MAPLANG_TASKLOOPER_H_
