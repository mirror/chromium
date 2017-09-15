// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "simple_api.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/run_loop.h"
#include "base/task_runner.h"
#include "base/task_scheduler/post_task.h"

#include "base_task_initialization.h"

// One of these three macros must be defined to indicate which TaskRunner
// implementation to use.
#if !defined(USE_MESSAGE_LOOP) && !defined(USE_SEQUENCED_TASK_RUNNER) && \
    !defined(USE_SINGLE_THREAD_TASK_RUNNER)
#error \
    "One of the following macro must be defined:\
  USE_MESSAGE_LOOP, USE_MESSAGE_LOOP or USE_SINGLE_THREAD_TASK_RUNNER"
#endif

namespace {

simple::BaseTaskInitialization sBaseTaskInit;

class MyMath : public simple::Math {
 public:
  MyMath() : task_runner_(sBaseTaskInit.task_runner()) {}

  int32_t Add(int32_t x, int32_t y) override {
    int32_t sum;
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&MyMath::DoAdd, x, y,
                                  base::BindOnce(&MyMath::StoreResult, &sum)));
    base::RunLoop().RunUntilIdle();
    return sum;
  }

 private:
  using AddCallback = base::OnceCallback<void(int32_t)>;

  static void DoAdd(int x, int y, AddCallback cb) { std::move(cb).Run(x + y); }

  static void StoreResult(int32_t* result, int32_t value) { *result = value; }

  base::TaskRunner* task_runner_;
};

class MyEcho : public simple::Echo {
 public:
  MyEcho() : task_runner_(sBaseTaskInit.task_runner()) {}

  std::string Ping(const std::string& str) override {
    std::string result;
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&MyEcho::DoPing, base::ConstRef(str),
                       base::BindOnce(&MyEcho::StoreResult, &result)));
    base::RunLoop().RunUntilIdle();
    return result;
  }

 private:
  using PingCallback = base::OnceCallback<void(std::string)>;

  static void DoPing(const std::string& str, PingCallback cb) {
    std::move(cb).Run(str);
  }

  static void StoreResult(std::string* result, std::string value) {
    *result = std::move(value);
  }

  base::TaskRunner* task_runner_;
};

}  // namespace

simple::Math* createMath() {
  return new MyMath();
}
simple::Echo* createEcho() {
  return new MyEcho();
}
const char* createAbstract() {
  return "use PostTask() with 2 callbacks and with "
#if defined(USE_SEQUENCED_TASK_RUNNER)
         "custom SequencedTaskRunner"
#elif defined(USE_SINGLE_THREAD_TASK_RUNNER)
         "custom SingleThreadTaskRunner"
#else
         "MessageLoop's TaskRunner"
#endif
         ".";
}
