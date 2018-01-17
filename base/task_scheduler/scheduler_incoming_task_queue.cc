// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/scheduler_incoming_task_queue.h"

#include <stddef.h>

#include <algorithm>
#include <iterator>

#include "base/message_loop/message_loop.h"
#include "base/task_scheduler/task.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/task_scheduler/task_tracker.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "build/build_config.h"

// Notes for January
// 1) MessageLoop TaskTracker tolerability (allow with and without)
//    This allows Threads to Terminate.
//    Need to track which tasks the task tracker allowed and which ones were not
//    allowed
//    How does this work with delayed tasks? Disallow delayed tasks after
//    shutdown? Only have to worry about task tracker true -> false
//             or no task tracker at all
//    QUEUED -> TASK_TRACKER_EXECUTION -> TASK_TRACKER_SHUTDOWN
//           -> DIRECT_EXECUTION
//
// 2) Audit Lock Usage Here
//
// 3) Audit Other Data for Thread Safety

namespace base {
namespace internal {

namespace {

struct IncomingTasks {
  circular_deque<SchedulerIncomingTaskQueueTask> tasks;
  int high_resolution_tasks = 0;

  IncomingTasks() = default;

  IncomingTasks(IncomingTasks&& other) { *this = std::move(other); }

  IncomingTasks& operator=(IncomingTasks&& other) {
    tasks = std::move(other.tasks);
    high_resolution_tasks = other.high_resolution_tasks;
    other.high_resolution_tasks = 0;
    return *this;
  }

  void Prepend(IncomingTasks&& other) {
    tasks.insert(tasks.begin(), std::make_move_iterator(other.tasks.begin()),
                 std::make_move_iterator(other.tasks.end()));
    high_resolution_tasks += other.high_resolution_tasks;
    other.high_resolution_tasks = 0;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(IncomingTasks);
};

}  // namespace

class SchedulerIncomingTaskQueue::IncomingTaskRunner
    : public SingleThreadTaskRunner {
 public:
  explicit IncomingTaskRunner(SchedulerIncomingTaskQueue* outer)
      : outer_(outer) {}

  // SingleThreadTaskRunner:
  bool PostDelayedTask(const Location& from_here,
                       OnceClosure task,
                       TimeDelta delay) override {
    DCHECK(!task.is_null()) << from_here.ToString();
    return AddToIncomingQueue(from_here, std::move(task), delay,
                              Nestable::kNestable);
  }

  bool PostNonNestableDelayedTask(const Location& from_here,
                                  OnceClosure task,
                                  TimeDelta delay) override {
    DCHECK(!task.is_null()) << from_here.ToString();
    return AddToIncomingQueue(from_here, std::move(task), delay,
                              Nestable::kNonNestable);
  }

  bool RunsTasksInCurrentSequence() const override {
    AutoSchedulerLock lock(valid_thread_id_lock_);
    return valid_thread_id_ == PlatformThread::CurrentId();
  }

  // IncomingTaskRunner:
  void StartScheduling() {
    // THIS NEEDS A LOCK
    start_scheduling_tasks_ = true;
    if (!incoming_tasks_.tasks.empty())
      outer_->message_loop_->ScheduleWork();
  }

  void StopAcceptingTasks() { stop_accepting_tasks_.Set(); }

  void BindToCurrentThread() {
    AutoSchedulerLock lock(valid_thread_id_lock_);
    DCHECK_EQ(kInvalidThreadId, valid_thread_id_);
    valid_thread_id_ = PlatformThread::CurrentId();
  }

  void NotifyTaskTrackerOfCurrentTasks() {
    outer_->lock_.AssertAcquired();
    DCHECK(outer_->task_tracker_);
    for (auto& task : incoming_tasks_.tasks)
      task.task_tracker_aware = outer_->task_tracker_->WillPostTask(task);
  }

  void NotifyTaskTrackerOfTaskCancellation() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
    DCHECK(outer_->task_tracker_);
    // THIS NEEDS A LOCK
    for (auto& task : incoming_tasks_.tasks) {
      outer_->task_tracker_->MarkTaskSkipped(task);
      task.task_tracker_aware = false;
    }
  }

  IncomingTasks MoveAllTasks() {
    AutoSchedulerLock auto_lock(outer_->lock_);
    return MoveAllTasksLockRequired();
  }

  IncomingTasks MoveAllTasksLockRequired() {
    outer_->lock_.AssertAcquired();
    return std::move(incoming_tasks_);
  }

  void PrependIncomingTasks(IncomingTasks&& incoming_tasks) {
    AutoSchedulerLock auto_lock(outer_->lock_);
    incoming_tasks_.Prepend(std::move(incoming_tasks));
  }

 private:
  ~IncomingTaskRunner() override = default;

  // Returns true if the task was successfully added to the queue, otherwise
  // returns false. In all cases, the ownership of |task| is transferred to the
  // called method.
  bool AddToIncomingQueue(const Location& from_here,
                          OnceClosure closure,
                          TimeDelta delay,
                          Nestable nestable) {
    if (stop_accepting_tasks_.IsSet())
      return false;

    SchedulerIncomingTaskQueueTask task(from_here, std::move(closure),
                                        outer_->task_traits_, delay);
    task.single_thread_task_runner_ref = outer_->message_loop_->task_runner();
    task.nestable = nestable;

#if defined(OS_WIN)
    // We consider the task needs a high resolution timer if the delay is
    // more than 0 and less than 32ms. This caps the relative error to
    // less than 50% : a 33ms wait can wake at 48ms since the default
    // resolution on Windows is between 10 and 15ms.
    if (delay > TimeDelta() &&
        delay.InMilliseconds() < (2 * Time::kMinLowResolutionThresholdMs)) {
      task.is_high_res = true;
    }
#endif

    AutoSchedulerLock auto_lock(outer_->lock_);

    if (TaskScheduler::GetTaskSchedulerVersion() ==
            outer_->task_scheduler_version_ &&
        outer_->task_tracker_)
      task.task_tracker_aware = outer_->task_tracker_->WillPostTask(task);

    if (task.is_high_res)
      ++incoming_tasks_.high_resolution_tasks;

    bool was_empty = incoming_tasks_.tasks.empty();
    incoming_tasks_.tasks.push_back(std::move(task));

    if (was_empty && start_scheduling_tasks_) {
      // This would be a good place to check for WillScheduleSequence() for
      // background priority deferrals. However, since MessageLoops will
      // generally be higher priority than that for now, we'll skip that check
      // and skip implementing the callback delegate.
      outer_->message_loop_->ScheduleWork();
    }
    return true;
  }

  const scoped_refptr<SchedulerIncomingTaskQueue> outer_;

  IncomingTasks incoming_tasks_;
  AtomicFlag stop_accepting_tasks_;
  bool start_scheduling_tasks_ = false;

  // ID of the thread |this| was created on.  Could be accessed on multiple
  // threads, protected by |valid_thread_id_lock_|.
  PlatformThreadId valid_thread_id_ = kInvalidThreadId;

  mutable SchedulerLock valid_thread_id_lock_;

  DISALLOW_COPY_AND_ASSIGN(IncomingTaskRunner);
};

SchedulerIncomingTaskQueue::SchedulerIncomingTaskQueue(
    MessageLoop* message_loop,
    const TaskTraits& task_traits)
    : message_loop_(message_loop),
      triage_tasks_(this),
      delayed_tasks_(this),
      deferred_tasks_(this),
      incoming_task_runner_(new IncomingTaskRunner(this)),
      task_traits_(task_traits) {
  // The constructing sequence is not necessarily the running sequence in the
  // case of base::Thread.
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

// This definition cannot be inlined to the header file as the header does not
// contain the IncomingTaskRunner->SingleThreadTaskRunner relationship.
scoped_refptr<SingleThreadTaskRunner>
SchedulerIncomingTaskQueue::incoming_task_runner() {
  return incoming_task_runner_;
}

bool SchedulerIncomingTaskQueue::IsIdleForTesting() {
  return !triage_tasks_.HasTasks();
}

void SchedulerIncomingTaskQueue::WillDestroyCurrentMessageLoop() {
  incoming_task_runner_->StopAcceptingTasks();
}

void SchedulerIncomingTaskQueue::StartScheduling() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  incoming_task_runner_->StartScheduling();
}

void SchedulerIncomingTaskQueue::OnRunLoopStarting() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!TaskScheduler::GetInstance())
    return;

  task_scheduler_version_ = TaskScheduler::GetTaskSchedulerVersion();
  TaskScheduler::GetInstance()->SetTaskTrackerForSchedulerIncomingTaskQueue(
      this);
}

void SchedulerIncomingTaskQueue::OnRunLoopStopping() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // When a run loop ends, all remaining tasks are effectively cancelled, and
  // any new incoming tasks should be queued for the next run loop session
  // should it occur.
  SetTaskTracker(nullptr);
}

void SchedulerIncomingTaskQueue::RunTask(SchedulerIncomingTaskQueueTask* task) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (task->task_tracker_aware) {
    task_tracker_->DirectlyRunTask(task);
  } else {
    task_annotator_.RunTask("MessageLoop::PostTask", task);
  }
}

void SchedulerIncomingTaskQueue::RecordTaskCancelled(
    SchedulerIncomingTaskQueueTask* task) {
  if (task->task_tracker_aware) {
    DCHECK(task_tracker_);
    task_tracker_->MarkTaskSkipped(*task);
    task->task_tracker_aware = false;
  }
}

SchedulerIncomingTaskQueue::TriageQueue::TriageQueue(
    SchedulerIncomingTaskQueue* outer)
    : outer_(outer) {}

SchedulerIncomingTaskQueue::TriageQueue::~TriageQueue() = default;

const SchedulerIncomingTaskQueueTask&
SchedulerIncomingTaskQueue::TriageQueue::Peek() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  ReloadFromIncomingTaskRunnerIfEmpty();
  return triage_queue_.front();
}

SchedulerIncomingTaskQueueTask SchedulerIncomingTaskQueue::TriageQueue::Pop() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  ReloadFromIncomingTaskRunnerIfEmpty();
  SchedulerIncomingTaskQueueTask front = std::move(triage_queue_.front());
  triage_queue_.pop_front();

  if (front.is_high_res)
    --outer_->pending_high_res_tasks_;

  return front;
}

// Whether this queue has tasks after reloading from the incoming queue.
bool SchedulerIncomingTaskQueue::TriageQueue::HasTasks() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  ReloadFromIncomingTaskRunnerIfEmpty();
  return !triage_queue_.empty();
}

void SchedulerIncomingTaskQueue::TriageQueue::Clear() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  // Previously, MessageLoop would delete all tasks including delayed and
  // deferred tasks in a single round before attempting to reload from the
  // incoming queue to see if more tasks remained. This gave it a chance to
  // assess whether or not clearing should continue. As a result, while
  // reloading is automatic for getting and seeing if tasks exist, it is not
  // automatic for Clear().
  while (!triage_queue_.empty()) {
    SchedulerIncomingTaskQueueTask task = std::move(triage_queue_.front());
    triage_queue_.pop_front();

    if (task.is_high_res)
      --outer_->pending_high_res_tasks_;

    outer_->RecordTaskCancelled(&task);
  }
}

void SchedulerIncomingTaskQueue::TriageQueue::
    ReloadFromIncomingTaskRunnerIfEmpty() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  if (triage_queue_.empty()) {
    IncomingTasks incoming_tasks =
        outer_->incoming_task_runner_->MoveAllTasks();
    triage_queue_ = std::move(incoming_tasks.tasks);
    outer_->pending_high_res_tasks_ += incoming_tasks.high_resolution_tasks;
  }
}

void SchedulerIncomingTaskQueue::TriageQueue::
    NotifyTaskTrackerOfCurrentTasks() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  for (auto& task : triage_queue_)
    task.task_tracker_aware = outer_->task_tracker_->WillPostTask(task);
}

void SchedulerIncomingTaskQueue::TriageQueue::
    NotifyTaskTrackerOfTaskCancellation() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  for (auto& task : triage_queue_)
    outer_->RecordTaskCancelled(&task);
}

SchedulerIncomingTaskQueue::DelayedQueue::DelayedQueue(
    SchedulerIncomingTaskQueue* outer)
    : outer_(outer) {}

SchedulerIncomingTaskQueue::DelayedQueue::~DelayedQueue() = default;

const SchedulerIncomingTaskQueueTask&
SchedulerIncomingTaskQueue::DelayedQueue::Peek() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  return delayed_queue_.front();
}

SchedulerIncomingTaskQueueTask SchedulerIncomingTaskQueue::DelayedQueue::Pop() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  std::pop_heap(delayed_queue_.begin(), delayed_queue_.end());
  SchedulerIncomingTaskQueueTask delayed_task =
      std::move(delayed_queue_.back());
  delayed_queue_.pop_back();

  if (delayed_task.is_high_res)
    --outer_->pending_high_res_tasks_;
  return delayed_task;
}

bool SchedulerIncomingTaskQueue::DelayedQueue::HasTasks() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  while (!delayed_queue_.empty() && Peek().task.IsCancelled()) {
    SchedulerIncomingTaskQueueTask task = Pop();
    outer_->RecordTaskCancelled(&task);
  }
  return !delayed_queue_.empty();
}

void SchedulerIncomingTaskQueue::DelayedQueue::Clear() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  while (!delayed_queue_.empty()) {
    SchedulerIncomingTaskQueueTask task = Pop();
    outer_->RecordTaskCancelled(&task);
  }
}

void SchedulerIncomingTaskQueue::DelayedQueue::Push(
    SchedulerIncomingTaskQueueTask pending_task) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);

  if (pending_task.is_high_res)
    ++outer_->pending_high_res_tasks_;

  delayed_queue_.push_back(std::move(pending_task));
  std::push_heap(delayed_queue_.begin(), delayed_queue_.end());
}

void SchedulerIncomingTaskQueue::DelayedQueue::
    NotifyTaskTrackerOfCurrentTasks() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  for (auto& task : delayed_queue_)
    task.task_tracker_aware = outer_->task_tracker_->WillPostTask(task);
}

void SchedulerIncomingTaskQueue::DelayedQueue::
    NotifyTaskTrackerOfTaskCancellation() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  for (auto& task : delayed_queue_)
    outer_->RecordTaskCancelled(&task);
}

SchedulerIncomingTaskQueue::DeferredQueue::DeferredQueue(
    SchedulerIncomingTaskQueue* outer)
    : outer_(outer) {}

SchedulerIncomingTaskQueue::DeferredQueue::~DeferredQueue() = default;

const SchedulerIncomingTaskQueueTask&
SchedulerIncomingTaskQueue::DeferredQueue::Peek() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  return deferred_queue_.front();
}

SchedulerIncomingTaskQueueTask
SchedulerIncomingTaskQueue::DeferredQueue::Pop() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  SchedulerIncomingTaskQueueTask front = std::move(deferred_queue_.front());
  deferred_queue_.pop_front();
  return front;
}

// Whether this queue has tasks after reloading from the incoming queue.
bool SchedulerIncomingTaskQueue::DeferredQueue::HasTasks() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  return !deferred_queue_.empty();
}

void SchedulerIncomingTaskQueue::DeferredQueue::Clear() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  while (!deferred_queue_.empty()) {
    SchedulerIncomingTaskQueueTask task = std::move(deferred_queue_.front());
    deferred_queue_.pop_front();
    // The caller may never have run the RunLoop. There's no need to notify the
    // task tracker since we didn't let the TaskTracker know about these tasks.
    outer_->RecordTaskCancelled(&task);
  }
}

void SchedulerIncomingTaskQueue::DeferredQueue::
    NotifyTaskTrackerOfCurrentTasks() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  for (auto& task : deferred_queue_)
    task.task_tracker_aware = outer_->task_tracker_->WillPostTask(task);
}

void SchedulerIncomingTaskQueue::DeferredQueue::
    NotifyTaskTrackerOfTaskCancellation() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  for (auto& task : deferred_queue_)
    outer_->RecordTaskCancelled(&task);
}

void SchedulerIncomingTaskQueue::DeferredQueue::Push(
    SchedulerIncomingTaskQueueTask pending_task) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  deferred_queue_.push_back(std::move(pending_task));
}

SchedulerIncomingTaskQueue::~SchedulerIncomingTaskQueue() = default;

void SchedulerIncomingTaskQueue::BindToCurrentThread() {
  incoming_task_runner_->BindToCurrentThread();
}

void SchedulerIncomingTaskQueue::SetTaskTracker(TaskTracker* task_tracker) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  AutoSchedulerLock auto_lock(lock_);
  IncomingTasks snapshotted_tasks;
  if (task_tracker_ != task_tracker) {
    if (!task_tracker) {
      {
        AutoSchedulerUnlock auto_unlock(lock_);
        triage_tasks_.NotifyTaskTrackerOfTaskCancellation();
        delayed_tasks_.NotifyTaskTrackerOfTaskCancellation();
        deferred_tasks_.NotifyTaskTrackerOfTaskCancellation();
      }
      // Because cancelling a task acquires a SchedulerLock in the task tracker
      // that can't know about this class, we need to do some dancing to
      // properly cancel the tasks in the incoming task runner.
      //
      // The goal is to snapshot+move the incoming tasks onto this sequence and
      // then prepend it back to the incoming tasks.
      snapshotted_tasks = incoming_task_runner_->MoveAllTasksLockRequired();
    }
    TaskTracker* old_task_tracker = task_tracker_;
    task_tracker_ = task_tracker;
    if (old_task_tracker && !snapshotted_tasks.tasks.empty()) {
      AutoSchedulerUnlock auto_unlock(lock_);
      for (auto& task : snapshotted_tasks.tasks) {
        old_task_tracker->MarkTaskSkipped(task);
        task.task_tracker_aware = false;
      }
      incoming_task_runner_->PrependIncomingTasks(std::move(snapshotted_tasks));
    }
    if (task_tracker) {
      incoming_task_runner_->NotifyTaskTrackerOfCurrentTasks();
      triage_tasks_.NotifyTaskTrackerOfCurrentTasks();
      delayed_tasks_.NotifyTaskTrackerOfCurrentTasks();
      deferred_tasks_.NotifyTaskTrackerOfCurrentTasks();
    }
  }
}

}  // namespace internal
}  // namespace base
