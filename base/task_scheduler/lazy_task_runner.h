// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_SCHEDULER_LAZY_TASK_RUNNER_H_
#define BASE_TASK_SCHEDULER_LAZY_TASK_RUNNER_H_

#include "base/lazy_instance.h"

#include "base/lazy_instance.h"
#include "base/macros.h"
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
//
// A base::Lazy(Sequenced|SingleThread|COMSTA)TaskRunner releases its TaskRunner
// when a base::test::ScopedTaskEnvironment is destroyed. This is because
// destroying a base::test::ScopedTaskEnvironment destroys the current
// TaskScheduler, which in turn invalidates all TaskScheduler TaskRunners.
// Destroying the current TaskScheduler through other means than destroying a
// base::test::ScopedTaskEnvironment is not a supported use case (all tests
// should use base::test::ScopedTaskEnvironment and production code should not
// destroy TaskScheduler).

namespace base {
namespace internal {

template <typename TaskRunnerType>
class LazyTaskRunner {
 public:

  scoped_refptr<TaskRunnerType> Get() { return nullptr; }

 private:
  struct Members {
    SchedulerLock lock;
    scoped_refptr<TaskRunnerType> task_runner;
  };

  // Encapsulates members so that LazyTaskRunner can be constructed trivially.
  LazyInstance<Members> members_;

  TaskTraits traits_;
};

/*

template <typename TaskRunnerType>
class LazyTaskRunner {
 public:
  constexpr LazyTaskRunner(const TaskTraits& traits) : traits_(traits) {}

  int tutu();

  // Returns the TaskRunner reference held by this instance. Creates a
  // TaskRunner and keeps a reference to it if no reference was previously held
  // by this instance.
  scoped_refptr<TaskRunnerType> Get();

 public:
  struct Members {
    SchedulerLock lock;
    scoped_refptr<TaskRunnerType> task_runner;
  };

  // Encapsulates members so that LazyTaskRunner can be constructed trivially.
  LazyInstance<Members> members_;

  TaskTraits traits_;
};
/*
template <>
class LazyTaskRunner<SingleThreadTaskRunner> {
 public:
  constexpr LazyTaskRunner(TaskTraits traits, bool com_sta, SingleThreadTaskRunnerThreadMode thread_mode)
      : traits_(traits), com_sta_(com_sta), thread_mode_(thread_mode) {}

 private:
   TaskTraits traits_;
  bool com_sta_;
    SingleThreadTaskRunnerThreadMode thread_mode_;
};*/

}  // namespace internal

//using LazySequencedTaskRunner = internal::LazyTaskRunner<SequencedTaskRunner>;
//using LazySingleThreadTaskRunner = internal::LazyTaskRunner<SingleThreadTaskRunner>;

}  // namespace base

#endif  // BASE_TASK_SCHEDULER_LAZY_TASK_RUNNER_H_
