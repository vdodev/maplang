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

#ifndef __MAPLANG_UVLOOPRUNNER_H_
#define __MAPLANG_UVLOOPRUNNER_H_

#include <uv.h>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace maplang {

class UvLoopRunner final {
 public:
  UvLoopRunner();

  std::shared_ptr<uv_loop_t> getLoop() const;

  void waitForExit();

 private:
  const std::shared_ptr<uv_loop_t> mUvLoop;
  std::thread mThread;
  std::mutex mMutex;
  std::condition_variable mThreadStopped;
  bool mStopped = false;
  uv_async_s mUvAsync;
};

}  // namespace maplang

#endif //__MAPLANG_UVLOOPRUNNER_H_
