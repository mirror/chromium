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

namespace {

// A static instance ensures this is initialized and finalization at
// library load / unload time, respectively.
simple::BaseTaskInitialization sBaseTaskInit;

class MyMath : public simple::Math {
 public:
  MyMath() : task_runner_(sBaseTaskInit.task_runner()) {}

  int32_t Add(int32_t x, int32_t y) override {
    int32_t sum;
    task_runner_->PostTask(FROM_HERE, base::BindOnce(&DoAdd, x, y, &sum));
    base::RunLoop().RunUntilIdle();
    return sum;
  }

 private:
  static void DoAdd(int32_t x, int32_t y, int32_t* sum) { *sum = x + y; }

  base::TaskRunner* task_runner_;
};

class MyEcho : public simple::Echo {
 public:
  MyEcho() : task_runner_(sBaseTaskInit.task_runner()) {}

  std::string Ping(const std::string& str) override {
    std::string result;
    task_runner_->PostTask(FROM_HERE, base::BindOnce(&DoPing, str, &result));
    base::RunLoop().RunUntilIdle();
    return result;
  }

 private:
  static void DoPing(const std::string& str, std::string* result) {
    *result = str;
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
  return "use PostTask() with "
#if defined(USE_SEQUENCED_TASK_RUNNER)
         "custom SequencedTaskRunner"
#elif defined(USE_SINGLE_THREAD_TASK_RUNNER)
         "custom SingleThreadTaskRunner"
#else
         "MessageLoop's TaskRunner"
#endif
         ".";
}
