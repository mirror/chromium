// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

#include "gin/v8_taskrunner.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_scheduler.h"
#include "gin/public/isolate_holder.h"

namespace gin {

V8ForegroundTaskRunner::V8ForegroundTaskRunner(
    v8::Isolate* isolate,
    IsolateHolder::AccessMode access_mode,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : isolate_(isolate), access_mode_(access_mode), task_runner_(task_runner) {}

V8ForegroundTaskRunner::~V8ForegroundTaskRunner() {}

namespace {

constexpr base::TaskTraits kBackgroundThreadTaskTraits = {
    base::TaskPriority::USER_VISIBLE};

void RunWithLocker(v8::Isolate* isolate, v8::Task* task) {
  v8::Locker lock(isolate);
  task->Run();
}

class IdleTaskWithLocker : public v8::IdleTask {
 public:
  IdleTaskWithLocker(v8::Isolate* isolate, std::unique_ptr<v8::IdleTask> task)
      : isolate_(isolate), task_(std::move(task)) {}

  ~IdleTaskWithLocker() override = default;

  // v8::IdleTask implementation.
  void Run(double deadline_in_seconds) override {
    v8::Locker lock(isolate_);
    task_->Run(deadline_in_seconds);
  }

 private:
  v8::Isolate* isolate_;
  std::unique_ptr<v8::IdleTask> task_;

  DISALLOW_COPY_AND_ASSIGN(IdleTaskWithLocker);
};

}  // namespace

void V8ForegroundTaskRunner::PostTask(std::unique_ptr<v8::Task> task) {
  if (access_mode_ == IsolateHolder::kUseLocker) {
    task_runner_->PostTask(FROM_HERE,
                           base::Bind(RunWithLocker, base::Unretained(isolate_),
                                      base::Owned(task.release())));
  } else {
    task_runner_->PostTask(
        FROM_HERE, base::Bind(&v8::Task::Run, base::Owned(task.release())));
  }
}

void V8ForegroundTaskRunner::PostDelayedTask(std::unique_ptr<v8::Task> task,
                                             double delay_in_seconds) {
  if (access_mode_ == IsolateHolder::kUseLocker) {
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(RunWithLocker, base::Unretained(isolate_),
                   base::Owned(task.release())),
        base::TimeDelta::FromSecondsD(delay_in_seconds));
  } else {
    task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(&v8::Task::Run, base::Owned(task.release())),
        base::TimeDelta::FromSecondsD(delay_in_seconds));
  }
}

void V8ForegroundTaskRunner::PostIdleTask(std::unique_ptr<v8::IdleTask> task) {
  DCHECK(IdleTasksEnabled());
  if (access_mode_ == IsolateHolder::kUseLocker) {
    idle_task_runner_->PostIdleTask(
        new IdleTaskWithLocker(isolate_, std::move(task)));
  } else {
    idle_task_runner_->PostIdleTask(task.release());
  }
}

void V8ForegroundTaskRunner::EnableIdleTasks(
    std::unique_ptr<V8IdleTaskRunner> idle_task_runner) {
  idle_task_runner_ = std::move(idle_task_runner);
}

bool V8ForegroundTaskRunner::IdleTasksEnabled() {
  return idle_task_runner_.get() != nullptr;
}

void V8BackgroundTaskRunner::PostTask(std::unique_ptr<v8::Task> task) {
  base::PostTaskWithTraits(
      FROM_HERE, kBackgroundThreadTaskTraits,
      base::Bind(&v8::Task::Run, base::Owned(task.release())));
}

void V8BackgroundTaskRunner::PostDelayedTask(std::unique_ptr<v8::Task> task,
                                             double delay_in_seconds) {
  base::PostDelayedTaskWithTraits(
      FROM_HERE, kBackgroundThreadTaskTraits,
      base::Bind(&v8::Task::Run, base::Owned(task.release())),
      base::TimeDelta::FromSecondsD(delay_in_seconds));
}

void V8BackgroundTaskRunner::PostIdleTask(std::unique_ptr<v8::IdleTask> task) {
  NOTREACHED() << "Idle tasks are not supported on background threads.";
}

bool V8BackgroundTaskRunner::IdleTasksEnabled() {
  // No idle tasks on background threads.
  return false;
}

// static
size_t V8BackgroundTaskRunner::NumberOfAvailableBackgroundThreads() {
  return std::max(1, base::TaskScheduler::GetInstance()
                         ->GetMaxConcurrentNonBlockedTasksWithTraitsDeprecated(
                             kBackgroundThreadTaskTraits));
}

}  // namespace gin
