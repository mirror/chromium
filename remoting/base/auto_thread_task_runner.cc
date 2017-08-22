// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/base/auto_thread_task_runner.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"

namespace remoting {

AutoThreadTaskRunner::AutoThreadTaskRunner(base::OnceClosure stop_task)
    : task_runner_(base::ThreadTaskRunnerHandle::Get()),
      stop_task_(std::move(stop_task)) {
  DCHECK(task_runner_);
  DCHECK(stop_task_);
}

#if defined(OS_CHROMEOS)
AutoThreadTaskRunner::AutoThreadTaskRunner(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : task_runner_(std::move(task_runner)),
      stop_task_(base::Bind(&base::DoNothing)) {
  DCHECK(task_runner_);
  DCHECK(stop_task_);
}
#endif

bool AutoThreadTaskRunner::PostDelayedTask(
    const tracked_objects::Location& from_here,
    base::OnceClosure task,
    base::TimeDelta delay) {
  CHECK(task_runner_->PostDelayedTask(from_here, std::move(task), delay));
  return true;
}

bool AutoThreadTaskRunner::PostNonNestableDelayedTask(
    const tracked_objects::Location& from_here,
    base::OnceClosure task,
    base::TimeDelta delay) {
  CHECK(task_runner_->PostNonNestableDelayedTask(
      from_here, std::move(task), delay));
  return true;
}

bool AutoThreadTaskRunner::RunsTasksInCurrentSequence() const {
  return task_runner_->RunsTasksInCurrentSequence();
}

AutoThreadTaskRunner::~AutoThreadTaskRunner() {
  CHECK(task_runner_->PostTask(FROM_HERE, std::move(stop_task_)));
}

}  // namespace remoting
