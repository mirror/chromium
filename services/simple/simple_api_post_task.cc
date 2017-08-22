// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "simple_api.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_scheduler.h"

// One of these three macros must be defined to indicate which TaskRunner
// implementation to use.
#if !defined(USE_MESSAGE_LOOP) && !defined(USE_SEQUENCED_TASK_RUNNER) && \
    !defined(USE_SINGLE_THREAD_TASK_RUNNER)
#error \
    "One of the following macro must be defined:\
  USE_MESSAGE_LOOP, USE_MESSAGE_LOOP or USE_SINGLE_THREAD_TASK_RUNNER"
#endif

namespace {

// Implementation of the simple:: classes using a base::PostTask() call.

// Convenience class to initialize a message loop and a task scheduler on
// the current thread, otherwise base::PostTask() will CHECK() at runtime.
// Used as a static instance variable in order to ensure that it is
// initialized when the library is loaded, and finalized when it is
// unloaded.
class BaseInit {
 public:
  BaseInit() : loop_() {
    base::TaskScheduler::Create("simple_api");

    task_scheduler_ = base::TaskScheduler::GetInstance();

#if defined(USE_SEQUENCED_TASK_RUNNER)
    task_runner_ =
        base::CreateSequencedTaskRunnerWithTraits(base::TaskTraits());
#elif defined(USE_SINGLE_THREAD_TASK_RUNNER)
    task_runner_ = base::CreateSingleThreadTaskRunnerWithTraits(
        base::TaskTraits(), base::SingleThreadTaskRunnerThreadMode::SHARED);
#endif
  }

  ~BaseInit() {
    task_scheduler_->Shutdown();
    // The following is required to avoid a DCHECK() in the TaskScheduler
    // destructor. Even though this is not really a test.
    task_scheduler_->JoinForTesting();
    base::TaskScheduler::SetInstance(nullptr);
  }

  base::TaskRunner* task_runner() {
#if defined(USE_SEQUENCED_TASK_RUNNER) || defined(USE_SINGLE_THREAD_TASK_RUNNER)
    return task_runner_.get();
#else
    return loop_.task_runner().get();
#endif
  }

 private:
  base::MessageLoop loop_;
  base::TaskScheduler* task_scheduler_ = nullptr;
#if defined(USE_SEQUENCED_TASK_RUNNER)
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
#elif defined(USE_SINGLE_THREAD_TASK_RUNNER)
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
#endif
};

static BaseInit sBaseInit;

class MyMath : public simple::Math {
 public:
  MyMath()
      : x_(0),
        y_(0),
        sum_(0),
        callback_(base::Bind(&MyMath::DoAdd, base::Unretained(this))),
        task_runner_(sBaseInit.task_runner()) {}

  int32_t Add(int32_t x, int32_t y) override {
    x_ = x;
    y_ = y;

    task_runner_->PostTask(FROM_HERE, callback_);

    base::RunLoop().RunUntilIdle();
    return sum_;
  }

 private:
  void DoAdd() { sum_ = x_ + y_; }

  int32_t x_, y_, sum_;
  base::Closure callback_;
  base::TaskRunner* task_runner_;
};

class MyEcho : public simple::Echo {
 public:
  MyEcho()
      : callback_(base::Bind(&MyEcho::DoPing, base::Unretained(this))),
        task_runner_(sBaseInit.task_runner()) {}

  std::string Ping(const std::string& str) override {
    str_ = &str;
    task_runner_->PostTask(FROM_HERE, callback_);
    base::RunLoop().RunUntilIdle();
    return result_;
  }

 private:
  void DoPing() { result_ = *str_; }

  std::string result_;
  const std::string* str_ = nullptr;
  base::Closure callback_;
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
