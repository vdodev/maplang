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

#ifndef MAPLANG_UVLOOPRUNNER_H_
#define MAPLANG_UVLOOPRUNNER_H_

#include <uv.h>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

namespace maplang {

class UvLoopRunner final {
 public:
  UvLoopRunner();
  ~UvLoopRunner();

  std::shared_ptr<uv_loop_t> getLoop() const;

  void drain();
  bool waitForExit(
      const std::optional<std::chrono::milliseconds>& maxWait = {});

  std::thread::id getUvLoopThreadId() const { return mUvLoopThreadId; }

 private:
  std::shared_ptr<uv_loop_t> mUvLoop;
  std::thread mThread;
  std::mutex mMutex;
  std::condition_variable mThreadStopped;
  bool mStopped = false;
  uv_async_s mUvAsync;
  std::thread::id mUvLoopThreadId;
};

}  // namespace maplang

#endif  // MAPLANG_UVLOOPRUNNER_H_
