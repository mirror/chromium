// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/scheduler_incoming_task_queue.h"

#include "base/message_loop/message_loop.h"
#include "base/task_scheduler/task.h"
#include "base/task_scheduler/task_tracker.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"

namespace base {
namespace internal {

class SchedulerIncomingTaskQueue::IncomingTaskRunner
    : public SingleThreadTaskRunner {
 public:
  explicit IncomingTaskRunner(SchedulerIncomingTaskQueue* outer)
      : outer_(outer) {}

  bool PostDelayedTask(const Location& from_here,
                       OnceClosure task,
                       TimeDelta delay) override {
    DCHECK(!task.is_null()) << from_here.ToString();
    return outer_->AddToIncomingQueue(
        from_here, std::move(task), delay, Nestable::kNestable);
  }

  bool PostNonNestableDelayedTask(const Location& from_here,
                                  OnceClosure task,
                                  TimeDelta delay) override {
    DCHECK(!task.is_null()) << from_here.ToString();
    return outer_->AddToIncomingQueue(
        from_here, std::move(task), delay, Nestable::kNonNestable);
  }

  bool RunsTasksInCurrentSequence() const override {
    return outer_->RunsTasksInCurrentSequence();
  }

 private:
  ~IncomingTaskRunner() override = default;

  const scoped_refptr<SchedulerIncomingTaskQueue> outer_;

  DISALLOW_COPY_AND_ASSIGN(IncomingTaskRunner);
};

SchedulerIncomingTaskQueue::SchedulerIncomingTaskQueue(
    MessageLoop* message_loop,
    TaskTracker* task_tracker,
    const TaskTraits& task_traits)
    : message_loop_(message_loop),
      triage_tasks_(this),
      delayed_tasks_(this),
      deferred_tasks_(this),
      incoming_task_runner_(new IncomingTaskRunner(this)),
      execution_sequence_(new Sequence),
      task_tracker_(task_tracker),
      task_traits_(task_traits),
      valid_thread_id_(kInvalidThreadId) {
  // The constructing sequence is not necessarily the running sequence in the
  // case of base::Thread.
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

bool SchedulerIncomingTaskQueue::IsIdleForTesting() {
  AutoLock auto_lock(lock_);
  return !triage_queue_.empty();
}

void SchedulerIncomingTaskQueue::WillDestroyCurrentMessageLoop() {
  stop_accepting_tasks_.Set();
}

void SchedulerIncomingTaskQueue::StartScheduling() {
  AutoLock auto_lock(lock_);
  start_scheduling_tasks_ = true;
  if (!triage_queue_.empty()) {
    message_loop_->ScheduleWork();
  }
}

void SchedulerIncomingTaskQueue::RunTask(Task* task) {
  task_tracker_->RunTask(task, execution_sequence_.get(),
                         TaskTracker::TaskResetPolicy::kDoNotReset);
}

SchedulerIncomingTaskQueue::TriageQueue::TriageQueue(
    SchedulerIncomingTaskQueue* outer)
    : outer_(outer) {}

SchedulerIncomingTaskQueue::TriageQueue::~TriageQueue() = default;

const Task& SchedulerIncomingTaskQueue::TriageQueue::Peek() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  AutoLock auto_lock(outer_->lock_);
  return outer_->triage_queue_.front();
}

Task SchedulerIncomingTaskQueue::TriageQueue::Pop() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  AutoLock auto_lock(outer_->lock_);
  Task front = std::move(outer_->triage_queue_.front());
  outer_->triage_queue_.pop_front();
  return front;
}

// Whether this queue has tasks after reloading from the incoming queue.
bool SchedulerIncomingTaskQueue::TriageQueue::HasTasks() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  AutoLock auto_lock(outer_->lock_);
  return !outer_->triage_queue_.empty();
}

void SchedulerIncomingTaskQueue::TriageQueue::Clear() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  AutoLock auto_lock(outer_->lock_);
  while (!outer_->triage_queue_.empty()) {
    Task task = std::move(outer_->triage_queue_.front());
    outer_->triage_queue_.pop_front();
    AutoUnlock auto_unlock(outer_->lock_);
    outer_->task_tracker_->DestroyTask(std::move(task));
  }
}

SchedulerIncomingTaskQueue::DelayedQueue::DelayedQueue(
    SchedulerIncomingTaskQueue* outer)
    : outer_(outer) {}

SchedulerIncomingTaskQueue::DelayedQueue::~DelayedQueue() = default;

const Task& SchedulerIncomingTaskQueue::DelayedQueue::Peek() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  return delayed_queue_.top();
}

Task SchedulerIncomingTaskQueue::DelayedQueue::Pop() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  Task delayed_task = std::move(const_cast<Task&>(delayed_queue_.top()));
  delayed_queue_.pop();
  return delayed_task;
}

bool SchedulerIncomingTaskQueue::DelayedQueue::HasTasks() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  return !delayed_queue_.empty();
}

void SchedulerIncomingTaskQueue::DelayedQueue::Clear() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  while (!delayed_queue_.empty())
    Pop();
}

void SchedulerIncomingTaskQueue::DelayedQueue::Push(Task pending_task) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  delayed_queue_.push(std::move(pending_task));
}

SchedulerIncomingTaskQueue::DeferredQueue::DeferredQueue(
    SchedulerIncomingTaskQueue* outer)
    : outer_(outer) {}

SchedulerIncomingTaskQueue::DeferredQueue::~DeferredQueue() = default;

const Task& SchedulerIncomingTaskQueue::DeferredQueue::Peek() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  return deferred_queue_.front();
}

Task SchedulerIncomingTaskQueue::DeferredQueue::Pop() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  Task front = std::move(deferred_queue_.front());
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
  deferred_queue_.clear();
}

void SchedulerIncomingTaskQueue::DeferredQueue::Push(Task pending_task) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(outer_->sequence_checker_);
  deferred_queue_.push_back(std::move(pending_task));
}

SchedulerIncomingTaskQueue::~SchedulerIncomingTaskQueue() = default;

void SchedulerIncomingTaskQueue::BindToCurrentThread() {
  AutoLock lock(valid_thread_id_lock_);
  DCHECK_EQ(kInvalidThreadId, valid_thread_id_);
  valid_thread_id_ = PlatformThread::CurrentId();
}

bool SchedulerIncomingTaskQueue::AddToIncomingQueue(const Location& from_here,
                                                    OnceClosure closure,
                                                    TimeDelta delay,
                                                    Nestable nestable) {
  if (stop_accepting_tasks_.IsSet())
    return false;

  Task task(from_here, std::move(closure), task_traits_, delay);
  // MessageLoop::SetTaskRunner() can override the posting task runner.
  // Is this the right thing to do or should we suppress the DCHECK?
  task.single_thread_task_runner_ref = message_loop_->task_runner();
  task.nestable = nestable;

  if (!task_tracker_->WillPostTask(task))
    return false;

  AutoLock auto_lock(lock_);
  bool was_empty = triage_queue_.empty();
  triage_queue_.push_back(std::move(task));
  if (was_empty && start_scheduling_tasks_) {
    // This would be a good place to check for WillScheduleSequence() for
    // background priority deferrals. However, since MessageLoops will generally
    // be higher priority than that for now, we'll skip that check and skip
    // implementing the callback delegate.
    message_loop_->ScheduleWork();
  }
  return true;
}

bool SchedulerIncomingTaskQueue::RunsTasksInCurrentSequence() const {
  AutoLock lock(valid_thread_id_lock_);
  return valid_thread_id_ == PlatformThread::CurrentId();
}

}  // namespace internal
}  // namespace base
