// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_SCHEDULER_SCHEDULER_WORKER_POOL_IMPL_H_
#define BASE_TASK_SCHEDULER_SCHEDULER_WORKER_POOL_IMPL_H_

#include <stddef.h>

#include <memory>
#include <string>
#include <vector>

#include "base/base_export.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/atomic_flag.h"
#include "base/synchronization/condition_variable.h"
#include "base/task_scheduler/priority_queue.h"
#include "base/task_scheduler/scheduler_lock.h"
#include "base/task_scheduler/scheduler_worker.h"
#include "base/task_scheduler/scheduler_worker_pool.h"
#include "base/task_scheduler/scheduler_worker_stack.h"
#include "base/task_scheduler/sequence.h"
#include "base/task_scheduler/task.h"
#include "base/time/time.h"

namespace base {

class HistogramBase;
class SchedulerWorkerPoolParams;
class TaskTraits;

namespace internal {

class DelayedTaskManager;
class TaskTracker;

// A pool of workers that run Tasks.
//
// The pool doesn't create threads until Start() is called. Tasks can be posted
// at any time but will not run until after Start() is called.
//
// This class is thread-safe.
class BASE_EXPORT SchedulerWorkerPoolImpl : public SchedulerWorkerPool {
 public:
  // Constructs a pool without workers.
  //
  // |name| is used to label the pool's threads ("TaskScheduler" + |name| +
  // index) and histograms ("TaskScheduler." + histogram name + "." + |name| +
  // extra suffixes). |priority_hint| is the preferred thread priority; the
  // actual thread priority depends on shutdown state and platform capabilities.
  // |task_tracker| keeps track of tasks. |delayed_task_manager| handles tasks
  // posted with a delay.
  SchedulerWorkerPoolImpl(
      const std::string& name,
      ThreadPriority priority_hint,
      TaskTracker* task_tracker,
      DelayedTaskManager* delayed_task_manager);

  // Creates workers following the |params| specification, allowing existing and
  // future tasks to run. Can only be called once. CHECKs on failure.
  void Start(const SchedulerWorkerPoolParams& params);

  // Destroying a SchedulerWorkerPoolImpl returned by Create() is not allowed in
  // production; it is always leaked. In tests, it can only be destroyed after
  // JoinForTesting() has returned.
  ~SchedulerWorkerPoolImpl() override;

  // SchedulerWorkerPool:
  scoped_refptr<TaskRunner> CreateTaskRunnerWithTraits(
      const TaskTraits& traits) override;
  scoped_refptr<SequencedTaskRunner> CreateSequencedTaskRunnerWithTraits(
      const TaskTraits& traits) override;
  bool PostTaskWithSequence(std::unique_ptr<Task> task,
                            scoped_refptr<Sequence> sequence) override;
  void PostTaskWithSequenceNow(std::unique_ptr<Task> task,
                               scoped_refptr<Sequence> sequence) override;

  const HistogramBase* num_tasks_before_detach_histogram() const {
    return num_tasks_before_detach_histogram_;
  }

  const HistogramBase* num_tasks_between_waits_histogram() const {
    return num_tasks_between_waits_histogram_;
  }

  void GetHistograms(std::vector<const HistogramBase*>* histograms) const;

  // Examine the list of SchedulerWorkers for blocked tasks to check for and
  // carry out any necessary increases to |worker_capacity_|
  void AdjustWorkerCapacity();

  // Returns the maximum number of non-blocked tasks that can run concurrently
  // in this pool.
  //
  // TODO(fdoray): Remove this method. https://crbug.com/687264
  int GetMaxConcurrentNonBlockedTasksDeprecated() const;

  // Waits until at least |n| workers are idle.
  void WaitForWorkersIdleForTesting(size_t n);

  // Waits until all workers are idle.
  void WaitForAllWorkersIdleForTesting();

  // Joins all workers of this worker pool. Tasks that are already running are
  // allowed to complete their execution. New tasks should not posted after this
  // is called. TaskTracker::Flush() can be called to complete existing tasks,
  // which might otherwise post a task during JoinForTesting(). This can only be
  // called once.
  void JoinForTesting();

  // Disallows worker cleanup. If the suggested reclaim time is not
  // TimeDelta::Max(), the test must call this before JoinForTesting() to reduce
  // the chance of thread detachment during the process of joining all of the
  // threads, and as a result, threads running after JoinForTesting().
  void DisallowWorkerCleanupForTesting();

  // Returns the number of workers in this worker pool.
  size_t NumberOfWorkersForTesting();

  // Returns |worker_capacity_|.
  size_t WorkerCapacityForTesting();

  // Returns the number of workers in the |idle_workers_stack_|.
  size_t NumberOfIdleWorkersForTesting();

  // After being called, workers will need to be blocked for TimeDelta::Max()
  // before |worker_capacity_| can be increased.
  void MaximizeBlockedThresholdForTesting() {
    maximum_blocked_threshold_.Set();
  }

 private:
  class SchedulerWorkerDelegateImpl;

  SchedulerWorkerPoolImpl(const SchedulerWorkerPoolParams& params,
                          TaskTracker* task_tracker,
                          DelayedTaskManager* delayed_task_manager);

  // Waits until at least |n| workers are idle. |lock_| must be held to call
  // this function.
  void WaitForWorkersIdleAssertLockForTesting(size_t n);

  // Wakes up the last worker from this worker pool to go idle, if any.
  void WakeUpOneWorker();

  // Adds |worker| to |idle_workers_stack_|.
  void AddToIdleWorkersStack(SchedulerWorker* worker);

  // Peeks from |idle_workers_stack_|.
  const SchedulerWorker* PeekAtIdleWorkersStack() const;

  // Removes |worker| from |idle_workers_stack_|.
  void RemoveFromIdleWorkersStack(SchedulerWorker* worker);

  // Returns true if worker cleanup is permitted.
  bool CanWorkerCleanupForTesting();

  // Adds a new SchedulerWorker based on SchedulerWorkerPoolParams that were
  // passed into Start() and calls SchedulerWorker::Start() on the worker.
  // Returns the newly added worker if SchedulerWorker::Start() was successful
  // and nullptr otherwise. This cannot be called before Start(). This function
  // should only be called under the protection of |lock_|.
  scoped_refptr<SchedulerWorker> CreateRegisterAndStartSchedulerWorker();

  // Tries to add and start a new worker to the pool, but decrements
  // |worker_capacity_| if starting the worker fails. Returns the new worker
  // if successful and nullptr otherwise.
  scoped_refptr<SchedulerWorker> TryAddWorker();

  // Returns true if the |worker| is currently suspended.
  bool WorkerIsSuspended(const SchedulerWorker* worker) const;

  // Returns whether additional workers should be suspended.
  bool ShouldSuspendWorkers() const;

  // Returns the minimum amount of time a SchedulerWorker must be blocked before
  // it is considered blocked by the SchedulerWorkerPoolImpl.
  TimeDelta BlockedThreshold() const;

  const std::string name_;
  const ThreadPriority priority_hint_;

  // PriorityQueue from which all threads of this worker pool get work.
  PriorityQueue shared_priority_queue_;

  // Suggested reclaim time for workers. Initialized by Start(). Never modified
  // afterwards (i.e. can be read without synchronization after Start()).
  TimeDelta suggested_reclaim_time_;

  SchedulerBackwardCompatibility backward_compatibility_;

  // Synchronizes accesses to |workers_|, |worker_capacity_|,
  // |idle_workers_stack_|, |idle_workers_stack_cv_for_testing_|, and
  // |num_wake_ups_before_start_|. Has |shared_priority_queue_|'s lock as
  // its predecessor so that a worker can be pushed to |idle_workers_stack_|
  // within the scope of a Transaction (more details in GetWork()).
  mutable SchedulerLock lock_;

  // All workers owned by this worker pool.
  std::vector<scoped_refptr<SchedulerWorker>> workers_;

  // Workers can be added as needed up until there are |worker_capacity_|
  // workers.
  size_t worker_capacity_;

  // Initial value of |worker_capacity_| as set in Start().
  size_t initial_worker_capacity_;

  // Stack of idle workers. Initially, all workers are on this stack. A worker
  // is removed from the stack before its WakeUp() function is called and when
  // it receives work from GetWork() (a worker calls GetWork() when its sleep
  // timeout expires, even if its WakeUp() method hasn't been called). A worker
  // is pushed on this stack when it receives nullptr from GetWork().
  SchedulerWorkerStack idle_workers_stack_;

  // Signaled when a worker is added to the idle workers stack.
  std::unique_ptr<ConditionVariable> idle_workers_stack_cv_for_testing_;

  // Number of wake ups that occurred before Start(). Never modified after
  // Start() (i.e. can be read without synchronization after Start()).
  int num_wake_ups_before_start_ = 0;

  AtomicFlag maximum_blocked_threshold_;

  // Signaled once JoinForTesting() has returned.
  WaitableEvent join_for_testing_returned_;

  // Indicates to the delegates that workers are not permitted to cleanup.
  AtomicFlag worker_cleanup_disallowed_;

#if DCHECK_IS_ON()
  // Set at the start of JoinForTesting().
  AtomicFlag join_for_testing_started_;
#endif

  // TaskScheduler.NumTasksBeforeDetach.[worker pool name] histogram.
  // Intentionally leaked.
  HistogramBase* const num_tasks_before_detach_histogram_;

  // TaskScheduler.NumTasksBetweenWaits.[worker pool name] histogram.
  // Intentionally leaked.
  HistogramBase* const num_tasks_between_waits_histogram_;

  TaskTracker* const task_tracker_;
  DelayedTaskManager* const delayed_task_manager_;

  friend class TaskSchedulerWorkerPoolBlockedUnblockedTest;

  DISALLOW_COPY_AND_ASSIGN(SchedulerWorkerPoolImpl);
};

}  // namespace internal
}  // namespace base

#endif  // BASE_TASK_SCHEDULER_SCHEDULER_WORKER_POOL_IMPL_H_
