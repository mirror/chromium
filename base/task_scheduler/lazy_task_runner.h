// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_SCHEDULER_LAZY_TASK_RUNNER_H_
#define BASE_TASK_SCHEDULER_LAZY_TASK_RUNNER_H_

#include <vector>

#include "base/atomicops.h"
#include "base/callback.h"
#include "base/lazy_instance.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/task_scheduler/scheduler_lock.h"
#include "base/task_scheduler/single_thread_task_runner_thread_mode.h"
#include "base/task_scheduler/task_traits.h"
#include "build/build_config.h"

// A base::Lazy(Sequenced|SingleThread|COMSTA)TaskRunner lazily creates a
// TaskScheduler TaskRunner.
//
// Example usage:
//
// namespace {
// base::LazySequencedTaskRunner g_task_runner(
//     base::TaskTraits(base::MayBlock()));
// }  // namespace
//
// void Function() {
//   // Different invocations of this function post to the same
//   // MayBlock() SequencedTaskRunner.
//   g_task_runner.Get()->PostTask(FROM_HERE, base::BindOnce(...));
// }

namespace base {
namespace internal {

template <typename TaskRunnerType>
class LazyTaskRunner {
 public:
  scoped_refptr<TaskRunnerType> Get();

  void Reset();

  // Effectively private: member data is only public to allow the linker to
  // statically initialize it and to maintain a POD class. DO NOT USE FROM
  // OUTSIDE THIS CLASS.

  // TaskTraits to create the TaskRunner.
  const TaskTraits private_traits_;

  // Can have 3 values:
  // - This instance does not hold a TaskRunner: 0
  // - This instance is creating a TaskRunner: kLazyInstanceStateCreating
  // - This instance holds a TaskRunner: Pointer to the TaskRunner.
  subtle::AtomicWord private_state_;
};

}  // namespace internal

using LazySequencedTaskRunner = internal::LazyTaskRunner<SequencedTaskRunner>;

class BASE_EXPORT ScopedLazyTaskRunnerList {
 public:
  ScopedLazyTaskRunnerList();
  ~ScopedLazyTaskRunnerList();

 private:
  friend class internal::LazyTaskRunner<SequencedTaskRunner>;

  void AddCallback(OnceClosure callback);

  internal::SchedulerLock lock_;
  std::vector<OnceClosure> callbacks_;
};

}  // namespace base

#endif  // BASE_TASK_SCHEDULER_LAZY_TASK_RUNNER_H_
