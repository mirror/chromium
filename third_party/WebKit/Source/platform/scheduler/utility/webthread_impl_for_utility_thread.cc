// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/utility/webthread_impl_for_utility_thread.h"

#include "base/threading/thread_task_runner_handle.h"
#include "platform/WebTaskRunner.h"

namespace blink {
namespace scheduler {

class WebThreadImplForUtilityThread::WebTaskRunnerImplForUtilityThread
    : public blink::WebTaskRunner {
 public:
  WebTaskRunnerImplForUtilityThread(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner)
      : task_runner_(std::move(task_runner)) {}

  double MonotonicallyIncreasingVirtualTimeSeconds() const override {
    NOTIMPLEMENTED();
    return 0;
  }

  bool RunsTasksInCurrentSequence() const override {
    return task_runner_->RunsTasksInCurrentSequence();
  }

  bool PostNonNestableDelayedTask(const base::Location& location,
                                  base::OnceClosure task,
                                  base::TimeDelta delay) override {
    return task_runner_->PostNonNestableDelayedTask(location, std::move(task),
                                                    delay);
  }

  bool PostDelayedTask(const base::Location& location,
                       base::OnceClosure task,
                       base::TimeDelta delay) override {
    return task_runner_->PostDelayedTask(location, std::move(task), delay);
  }

 private:
  ~WebTaskRunnerImplForUtilityThread() override {}
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
};

WebThreadImplForUtilityThread::WebThreadImplForUtilityThread()
    : task_runner_(base::ThreadTaskRunnerHandle::Get()),
      web_task_runner_(new WebTaskRunnerImplForUtilityThread(task_runner_)),
      thread_id_(base::PlatformThread::CurrentId()) {}

WebThreadImplForUtilityThread::~WebThreadImplForUtilityThread() {}

blink::WebScheduler* WebThreadImplForUtilityThread::Scheduler() const {
  NOTIMPLEMENTED();
  return nullptr;
}

blink::PlatformThreadId WebThreadImplForUtilityThread::ThreadId() const {
  return thread_id_;
}

scoped_refptr<base::SingleThreadTaskRunner>
WebThreadImplForUtilityThread::GetTaskRunner() const {
  return task_runner_;
}

scheduler::SingleThreadIdleTaskRunner*
WebThreadImplForUtilityThread::GetIdleTaskRunner() const {
  NOTIMPLEMENTED();
  return nullptr;
}

void WebThreadImplForUtilityThread::Init() {}

WebTaskRunner* WebThreadImplForUtilityThread::GetWebTaskRunner() {
  return web_task_runner_.get();
}

}  // namespace scheduler
}  // namespace blink
