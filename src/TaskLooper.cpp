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

#include "maplang/TaskLooper.h"

using namespace std;

TaskLooper::TaskLooper(
    const std::shared_ptr<uv_loop_t>& uvLoop,
    std::function<void()>&& task)
    : mUvLoop(uvLoop), mTask(move(task)), mStarted(false) {
  uv_idle_init(mUvLoop.get(), &mUvIdle);

  mUvIdle.data = this;
}

TaskLooper::~TaskLooper() { requestStop(); }

void TaskLooper::start() {
  if (mStarted) {
    return;
  }

  uv_idle_start(&mUvIdle, loopWrapper);
}

void TaskLooper::requestStop() { uv_idle_stop(&mUvIdle); }

void TaskLooper::loopWrapper(uv_idle_t* handle) {
  TaskLooper* looper = reinterpret_cast<TaskLooper*>(handle->data);
  looper->loop();
}

void TaskLooper::loop() { mTask(); }
