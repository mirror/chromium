// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_BASE_AUTO_THREAD_TASK_RUNNER_H_
#define REMOTING_BASE_AUTO_THREAD_TASK_RUNNER_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"

namespace remoting {

// A wrapper around |SingleThreadTaskRunner| that provides automatic lifetime
// management, by posting a caller-supplied task to the underlying task runner
// when no more references remain.
class AutoThreadTaskRunner : public base::SingleThreadTaskRunner {
 public:
  // Constructs an instance of |AutoThreadTaskRunner| wrapping the task runner
  // on current thread. |stop_task| is posted to the task runner when the last
  // reference to the AutoThreadTaskRunner is dropped.
  explicit AutoThreadTaskRunner(base::OnceClosure stop_task);

#if defined(OS_CHROMEOS)
  // Makes a thin wrapper of a SingleThreadTaskRunner for Chrome OS only. We can
  // only use system threads. So making a thin wrapper of an existing
  // SingleThreadTaskRunner can reduce the platform inconsistency. When wrapping
  // a SingleThreadTaskRunner, we usually cannot stop it, so the |stop_task| is
  // always base::DoNothing.
  //
  // To ensure the underly thread can still execute tasks even after destructing
  // of AutoThreadTaskRunner(), caller needs to ensure the correct
  // base::RunLoop::QuitClosure() is posted to the |task_runner|, or the
  // |task_runner| won't stop before the application stops.
  //
  // But components in Chromoting do not need to use AutoThreadTaskRunner in the
  // first place, SingleThreadTaskRunner should be passed between components. So
  // this wrapper is not necessary.
  //
  // TODO(zijiehe): Replace references of AutoThreadTaskRunner with
  // SingleThreadTaskRunner in Chromoting.
  AutoThreadTaskRunner(scoped_refptr<base::SingleThreadTaskRunner> task_runner);
#endif

  // SingleThreadTaskRunner implementation
  bool PostDelayedTask(const tracked_objects::Location& from_here,
                       base::OnceClosure task,
                       base::TimeDelta delay) override;
  bool PostNonNestableDelayedTask(const tracked_objects::Location& from_here,
                                  base::OnceClosure task,
                                  base::TimeDelta delay) override;
  bool RunsTasksInCurrentSequence() const override;

  const scoped_refptr<base::SingleThreadTaskRunner>& task_runner() {
    return task_runner_;
  }

 private:
  ~AutoThreadTaskRunner() override;

  // The task runner on which AutoThreadTaskRunner is created.
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // Task posted to |task_runner_| to notify the caller that it may be stopped.
  base::OnceClosure stop_task_;

  DISALLOW_COPY_AND_ASSIGN(AutoThreadTaskRunner);
};

}  // namespace remoting

#endif  // REMOTING_BASE_AUTO_THREAD_TASK_RUNNER_H_
