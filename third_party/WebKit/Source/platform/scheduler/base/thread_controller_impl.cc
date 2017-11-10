// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/base/thread_controller_impl.h"

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/time/tick_clock.h"
#include "platform/scheduler/base/sequence.h"

namespace blink {
namespace scheduler {
namespace internal {

ThreadControllerImpl::ThreadControllerImpl(
    base::MessageLoop* message_loop,
    std::unique_ptr<base::TickClock> time_source)
    : message_loop_(message_loop),
      task_runner_(message_loop_->task_runner()),
      time_source_(std::move(time_source)),
      sequence_(nullptr) {
  do_work_closure_ = base::BindRepeating(&ThreadControllerImpl::DoWork,
                                         base::Unretained(this));
}

ThreadControllerImpl::ThreadControllerImpl(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    std::unique_ptr<base::TickClock> time_source)
    : message_loop_(nullptr),
      task_runner_(std::move(task_runner)),
      time_source_(std::move(time_source)),
      sequence_(nullptr) {
  do_work_closure_ = base::BindRepeating(&ThreadControllerImpl::DoWork,
                                         base::Unretained(this));
}

ThreadControllerImpl::~ThreadControllerImpl() {}

void ThreadControllerImpl::SetSequence(Sequence* sequence) {
  // DCHECK_MAIN_THREAD
  DCHECK(!sequence_ || !sequence);
  sequence_ = sequence;
}

void ThreadControllerImpl::ScheduleWork() {
  DCHECK(sequence_);
  cancelable_do_work_closure_.Reset(do_work_closure_);
  task_runner_->PostTask(FROM_HERE, do_work_closure_);
}

void ThreadControllerImpl::ScheduleDelayedWork(base::TimeDelta delay) {
  DCHECK(sequence_);
  cancelable_do_work_closure_.Reset(do_work_closure_);
  task_runner_->PostDelayedTask(FROM_HERE, do_work_closure_, delay);
}

void ThreadControllerImpl::CancelDelayedWork() {
  DCHECK(sequence_);
  cancelable_do_work_closure_.Cancel();
}

void ThreadControllerImpl::PostNonNestableTask(const base::Location& from_here,
                                               base::Closure task) {
  task_runner_->PostNonNestableTask(from_here, std::move(task));
}

bool ThreadControllerImpl::RunsTasksInCurrentSequence() {
  return task_runner_->RunsTasksInCurrentSequence();
}

base::TickClock* ThreadControllerImpl::GetClock() {
  return time_source_.get();
}

bool ThreadControllerImpl::IsNested() {
  DCHECK(RunsTasksInCurrentSequence());
  return base::RunLoop::IsNestedOnCurrentThread();
}

void ThreadControllerImpl::DoWork() {
  DCHECK(sequence_);
  base::PendingTask task = sequence_->TakeTask();
  task_annotator_.RunTask("ThreadControllerImpl::DoWork", &task);
  sequence_->DidRunTask();
}

void ThreadControllerImpl::AddNestingObserver(
    base::RunLoop::NestingObserver* observer) {
  base::RunLoop::AddNestingObserverOnCurrentThread(observer);
}

void ThreadControllerImpl::RemoveNestingObserver(
    base::RunLoop::NestingObserver* observer) {
  base::RunLoop::RemoveNestingObserverOnCurrentThread(observer);
}

}  // namespace internal
}  // namespace scheduler
}  // namespace blink
