// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop/message_loop.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_scheduler.h"

// Convenience class to initialize a message loop and a task scheduler on
// the current thread, otherwise base::PostTask() will CHECK() at runtime.
//
// In a shared library, this is best used as a static instance, to guarantee
// that this is initialized at library load time, and finalized at unload
// time.
//
// This supports three different TaskRunner types, determined by the
// definition of one of these three macros before including this header:
//
//     USE_MESSAGE_LOOP: Use the MessageLoop's own task runner.
//     USE_SEQUENCED_TASK_RUNNER: Create a custom sequenced task runner.
//     USE_SINGLE_THREAD_TASK_RUNNER: Create a custom single thread task runner.
//
// A pointer to the task runner can be retrieved with a call to task_runner().
//

#if !defined(USE_MESSAGE_LOOP) && !defined(USE_SEQUENCED_TASK_RUNNER) && \
    !defined(USE_SINGLE_THREAD_TASK_RUNNER)
#error \
    "One of the following macro must be defined:\
  USE_MESSAGE_LOOP, USE_MESSAGE_LOOP or USE_SINGLE_THREAD_TASK_RUNNER"
#endif

namespace simple {

class BaseTaskInitialization {
 public:
  BaseTaskInitialization();

  ~BaseTaskInitialization();

  base::TaskRunner* task_runner() const { return task_runner_.get(); }

 private:
  base::MessageLoop loop_;
  base::TaskScheduler* task_scheduler_ = nullptr;
  scoped_refptr<base::TaskRunner> task_runner_;
};

BaseTaskInitialization::BaseTaskInitialization() : loop_() {
  base::TaskScheduler::Create("simple_api");

  task_scheduler_ = base::TaskScheduler::GetInstance();

#if defined(USE_MESSAGE_LOOP)
  task_runner_ = loop_.task_runner();
#elif defined(USE_SEQUENCED_TASK_RUNNER)
  task_runner_ = base::CreateSequencedTaskRunnerWithTraits(base::TaskTraits());
#elif defined(USE_SINGLE_THREAD_TASK_RUNNER)
  task_runner_ = base::CreateSingleThreadTaskRunnerWithTraits(
      base::TaskTraits(), base::SingleThreadTaskRunnerThreadMode::SHARED);
#endif
}

BaseTaskInitialization::~BaseTaskInitialization() {
  task_runner_ = nullptr;

  task_scheduler_->Shutdown();

  // The following is required to avoid a DCHECK() in the TaskScheduler
  // destructor. Even though this is not really a test.
  task_scheduler_->JoinForTesting();
  base::TaskScheduler::SetInstance(nullptr);
}

}  // namespace simple
