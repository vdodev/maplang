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

#include "UvLoopRunner.h"
#include <uv.h>

#include <stdlib.h>
#include <string>

using namespace std;

namespace maplang {

static shared_ptr<uv_loop_t> createUvLoop() {
  auto uvLoop = shared_ptr<uv_loop_t>((uv_loop_t*) calloc(1, sizeof(uv_loop_t)),
                                      [](uv_loop_t* loop) {
                                        if (!loop) {
                                          return;
                                        }

                                        uv_loop_close(loop);
                                        free(loop);
                                      });

  if (uvLoop==nullptr) {
    throw bad_alloc();
  }

  int status = uv_loop_init(uvLoop.get());

  if (status!=0) {
    static constexpr size_t kErrorMessageBufLen = 128;
    char errorMessage[kErrorMessageBufLen];
    uv_strerror_r(status, errorMessage, sizeof(errorMessage));
    errorMessage[kErrorMessageBufLen - 1] = 0;

    throw runtime_error("Error " + to_string(status) + " initializing UV loop. " + errorMessage);
  }

  return uvLoop;
}

static void onAsync(uv_async_t* async) {}

UvLoopRunner::UvLoopRunner() : mUvLoop(createUvLoop()) {
  int status = uv_async_init(mUvLoop.get(), &mUvAsync, onAsync);
  if (status != 0) {
    static constexpr size_t kErrorMessageBufLen = 128;
    char errorMessage[kErrorMessageBufLen];
    uv_strerror_r(status, errorMessage, sizeof(errorMessage));
    errorMessage[kErrorMessageBufLen - 1] = 0;

    throw runtime_error("Error " + to_string(status) + " initializing UV async. " + errorMessage);
  }

  thread runnerThread([this](){
    uv_run(mUvLoop.get(), UV_RUN_DEFAULT);
    mStopped = true;
    mThreadStopped.notify_all();
  });

  runnerThread.detach();
  mThread.swap(runnerThread);
}

shared_ptr<uv_loop_t> UvLoopRunner::getLoop() const {
  return mUvLoop;
}

void UvLoopRunner::waitForExit() {
  unique_lock<mutex> l(mMutex);
  uv_async_send(&mUvAsync);
  mThreadStopped.wait(l, [this](){ return mStopped; });
}

}  // namespace maplang
