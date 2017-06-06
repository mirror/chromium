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

// Lazy(Sequenced|SingleThread|COMSTA)TaskRunner lazily creates a TaskRunner.
//
// Add a Lazy(Sequenced|SingleThread|COMSTA)TaskRunner variable to an anonymous
// namespace and use it to post multiple tasks to the same sequence/thread. No
// static initializer will be generated.
//
// IMPORTANT: If possible, add a
// scoped_refptr<(Sequenced|SingleThread)TaskRunner> member initialized with
// Create(Sequenced|SingleThread|COMSTA)TaskRunnerWithTraits() to an object
// accessible by all PostTask call site instead of using this API.
//
// Example usage 1:
//
// namespace {
// base::LazySequencedTaskRunner g_sequenced_task_runner =
//     base::LazySequencedTaskRunner(base::TaskTraits(base::MayBlock()));
// }  // namespace
//
// void SequencedFunction() {
//   // Different invocations of this function post to the same
//   // MayBlock() SequencedTaskRunner.
//   g_sequenced_task_runner.Get()->PostTask(FROM_HERE, base::BindOnce(...));
// }
//
// Example usage 2:
//
// namespace {
// base::LazySingleThreadTaskRunner g_single_thread_task_runner =
//    base::LazySingleThreadTaskRunner(
//        base::TaskTraits(base::TaskPriority::BACKGROUND),
//        base::SingleThreadTaskRunnerThreadMode::SHARED);
// }  // namespace
//
// // Code from different files can access the SingleThreadTaskRunner via this
// // function.
// scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunner() {
//   return g_single_thread_task_runner;
// }
//
// Lazy(Sequenced|SingleThread|COMSTA)TaskRunner releases its TaskRunner when
// the base::test::ScopedTaskEnvironment in the scope of which the TaskRunner
// was created is deleted.

namespace base {
namespace internal {

template <typename TaskRunnerType, bool com_sta>
class BASE_EXPORT LazyTaskRunner {
 public:
  // |traits| are TaskTraits to use to create the TaskRunner. If this
  // LazyTaskRunner is specialized to create a SingleThreadTaskRunner,
  // |thread_mode| specifies whether the SingleThreadTaskRunner can share its
  // thread with other SingleThreadTaskRunner. Otherwise, it is unused.
  constexpr LazyTaskRunner(const TaskTraits& traits,
                           SingleThreadTaskRunnerThreadMode thread_mode =
                               SingleThreadTaskRunnerThreadMode::SHARED)
      : traits_(traits), thread_mode_(thread_mode) {}

  // Returns the TaskRunner held by this instance. Creates it if it didn't
  // already exist.
  scoped_refptr<TaskRunnerType> Get();

 private:
  // Releases the TaskRunner held by this instance.
  void Reset();

  // Creates and returns a new TaskRunner.
  scoped_refptr<TaskRunnerType> Create();

  // TaskTraits to create the TaskRunner.
  const TaskTraits traits_;

  // SingleThreadTaskRunnerThreadMode to create the TaskRunner.
  const SingleThreadTaskRunnerThreadMode thread_mode_;

  // Can have 3 values:
  // - This instance does not hold a TaskRunner: 0
  // - This instance is creating a TaskRunner: kLazyInstanceStateCreating
  // - This instance holds a TaskRunner: Pointer to the TaskRunner.
  subtle::AtomicWord state_ = 0;

  // No DISALLOW_COPY_AND_ASSIGN since that prevents static initialization with
  // Visual Studio (warning C4592: 'symbol will be dynamically initialized
  // (implementation limitation))'.
};

// When a LazyTaskRunner creates a TaskRunner, it adds a callback to the current
// ScopedLazyTaskRunnerList. Callbacks run when the ScopedLazyTaskRunnerList is
// destroyed.
class BASE_EXPORT ScopedLazyTaskRunnerList {
 public:
  ScopedLazyTaskRunnerList();
  ~ScopedLazyTaskRunnerList();

 private:
  friend class internal::LazyTaskRunner<SequencedTaskRunner, false>;
  friend class internal::LazyTaskRunner<SingleThreadTaskRunner, false>;

#if defined(OS_WIN)
  friend class internal::LazyTaskRunner<SingleThreadTaskRunner, true>;
#endif

  // Add |callback| to the list of callbacks to run on destruction.
  void AddCallback(OnceClosure callback);

  // Synchronizes accesses to |callbacks_|.
  internal::SchedulerLock lock_;

  // List of callbacks to run on destruction.
  std::vector<OnceClosure> callbacks_;

  DISALLOW_COPY_AND_ASSIGN(ScopedLazyTaskRunnerList);
};

}  // namespace internal

// Lazy SequencedTaskRunner.
using LazySequencedTaskRunner =
    internal::LazyTaskRunner<SequencedTaskRunner, false>;

// Lazy SingleThreadTaskRunner.
using LazySingleThreadTaskRunner =
    internal::LazyTaskRunner<SingleThreadTaskRunner, false>;

#if defined(OS_WIN)
// Lazy COM-STA enabled SingleThreadTaskRunner.
using LazyCOMSTATaskRunner =
    internal::LazyTaskRunner<SingleThreadTaskRunner, true>;
#endif

}  // namespace base

#endif  // BASE_TASK_SCHEDULER_LAZY_TASK_RUNNER_H_
