// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/base/thread_controller_impl.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/time/tick_clock.h"
#include "base/trace_event/trace_event.h"
#include "platform/scheduler/base/sequence.h"

namespace blink {
namespace scheduler {
namespace internal {

ThreadControllerImpl::ThreadControllerImpl(
    base::MessageLoop* message_loop,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    base::TickClock* time_source)
    : message_loop_(message_loop),
      task_runner_(task_runner),
      message_loop_task_runner_(message_loop ? message_loop->task_runner()
                                             : nullptr),
      time_source_(time_source),
      weak_factory_(this) {
  immediate_do_work_closure_ = base::BindRepeating(
      &ThreadControllerImpl::DoWork, weak_factory_.GetWeakPtr(),
      Sequence::WorkType::kImmediate);
  delayed_do_work_closure_ = base::BindRepeating(&ThreadControllerImpl::DoWork,
                                                 weak_factory_.GetWeakPtr(),
                                                 Sequence::WorkType::kDelayed);

  if (message_loop_)
    base::RunLoop::AddNestingObserverOnCurrentThread(this);
}

ThreadControllerImpl::~ThreadControllerImpl() {
  if (message_loop_)
    base::RunLoop::RemoveNestingObserverOnCurrentThread(this);
}

std::unique_ptr<ThreadControllerImpl> ThreadControllerImpl::Create(
    base::MessageLoop* message_loop,
    base::TickClock* time_source) {
  return base::WrapUnique(new ThreadControllerImpl(
      message_loop, message_loop->task_runner(), time_source));
}

void ThreadControllerImpl::SetSequence(Sequence* sequence) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(sequence);
  DCHECK(!sequence_);
  sequence_ = sequence;
}

void ThreadControllerImpl::ScheduleWork() {
  DCHECK(sequence_);
  base::AutoLock lock(any_sequence_lock_);
  // Don't post a DoWork if there's an immediate DoWork in flight or if we're
  // inside a top level DoWork. We can rely on a continuation being posted as
  // needed.
  if (any_sequence().immediate_do_work_posted ||
      (any_sequence().do_work_running_count > any_sequence().nesting_depth)) {
    return;
  }
  any_sequence().immediate_do_work_posted = true;

  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "ThreadControllerImpl::ScheduleWork::PostTask");
  task_runner_->PostTask(FROM_HERE, immediate_do_work_closure_);
}

bool ThreadControllerImpl::DoWorkRunningOrPostedWithoutDelay() {
  if (main_sequence_only().do_work_running_count >
      main_sequence_only().nesting_depth) {
    return true;
  }

  base::AutoLock lock(any_sequence_lock_);
  return any_sequence().immediate_do_work_posted;
}

void ThreadControllerImpl::ScheduleDelayedWork(base::TimeDelta delay) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(sequence_);
  cancelable_delayed_do_work_closure_.Reset(delayed_do_work_closure_);

  task_runner_->PostDelayedTask(
      FROM_HERE, cancelable_delayed_do_work_closure_.callback(), delay);
}

void ThreadControllerImpl::CancelDelayedWork() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(sequence_);
  cancelable_delayed_do_work_closure_.Cancel();
}

bool ThreadControllerImpl::RunsTasksInCurrentSequence() {
  return task_runner_->RunsTasksInCurrentSequence();
}

base::TickClock* ThreadControllerImpl::GetClock() {
  return time_source_;
}

void ThreadControllerImpl::SetDefaultTaskRunner(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  if (!message_loop_)
    return;
  message_loop_->SetTaskRunner(task_runner);
}

void ThreadControllerImpl::RestoreDefaultTaskRunner() {
  if (!message_loop_)
    return;
  message_loop_->SetTaskRunner(message_loop_task_runner_);
}

void ThreadControllerImpl::DidQueueTask(const base::PendingTask& pending_task) {
  task_annotator_.DidQueueTask("TaskQueueManager::PostTask", pending_task);
}

void ThreadControllerImpl::DoWork(Sequence::WorkType work_type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(sequence_);

  {
    base::AutoLock lock(any_sequence_lock_);
    if (work_type == Sequence::WorkType::kImmediate)
      any_sequence().immediate_do_work_posted = false;
    any_sequence().do_work_running_count++;
  }

  main_sequence_only().do_work_running_count++;

  base::WeakPtr<ThreadControllerImpl> weak_ptr = weak_factory_.GetWeakPtr();
  base::TimeDelta delay_till_next_task = base::TimeDelta::Max();
  for (int i = 0; i < main_sequence_only().work_batch_size_; i++) {
    base::Optional<base::PendingTask> task = sequence_->TakeTask();
    if (!task) {
      // TakeTask will have posted a delayed continuation if needed.
      delay_till_next_task = base::TimeDelta::Max();
      break;
    }

    TRACE_TASK_EXECUTION("ThreadControllerImpl::DoWork", *task);
    task_annotator_.RunTask("ThreadControllerImpl::DoWork", &*task);

    if (!weak_ptr)
      return;

    delay_till_next_task = sequence_->DidRunTask();
    if (delay_till_next_task > base::TimeDelta())
      break;
  }

  main_sequence_only().do_work_running_count--;
  {
    base::AutoLock lock(any_sequence_lock_);
    any_sequence().do_work_running_count--;
    DCHECK_GE(any_sequence().do_work_running_count, 0);
    if (delay_till_next_task <= base::TimeDelta()) {
      if (!any_sequence().immediate_do_work_posted) {
        any_sequence().immediate_do_work_posted = true;
        task_runner_->PostTask(FROM_HERE, immediate_do_work_closure_);
      }
    } else if (delay_till_next_task < base::TimeDelta::Max()) {
      cancelable_delayed_do_work_closure_.Reset(delayed_do_work_closure_);
      task_runner_->PostDelayedTask(
          FROM_HERE, cancelable_delayed_do_work_closure_.callback(),
          delay_till_next_task);
    }
  }
}

void ThreadControllerImpl::AddNestingObserver(
    base::RunLoop::NestingObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  nesting_observer_ = observer;
}

void ThreadControllerImpl::RemoveNestingObserver(
    base::RunLoop::NestingObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(observer, nesting_observer_);
  nesting_observer_ = nullptr;
}

void ThreadControllerImpl::OnBeginNestedRunLoop() {
  main_sequence_only().nesting_depth++;
  {
    base::AutoLock lock(any_sequence_lock_);
    any_sequence().nesting_depth++;
    if (!any_sequence().immediate_do_work_posted) {
      any_sequence().immediate_do_work_posted = true;
      TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
                   "ThreadControllerImpl::OnBeginNestedRunLoop::PostTask");
      task_runner_->PostTask(FROM_HERE, immediate_do_work_closure_);
    }
  }
  if (nesting_observer_)
    nesting_observer_->OnBeginNestedRunLoop();
}

void ThreadControllerImpl::OnExitNestedRunLoop() {
  main_sequence_only().nesting_depth--;
  {
    base::AutoLock lock(any_sequence_lock_);
    any_sequence().nesting_depth--;
    DCHECK_GE(any_sequence().nesting_depth, 0);
  }
  if (nesting_observer_)
    nesting_observer_->OnExitNestedRunLoop();
}

void ThreadControllerImpl::SetWorkBatchSize(int work_batch_size) {
  main_sequence_only().work_batch_size_ = work_batch_size;
}

}  // namespace internal
}  // namespace scheduler
}  // namespace blink
