// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/test/lazy_scheduler_message_loop_delegate_for_tests.h"

#include <memory>
#include <utility>

#include "base/message_loop/message_loop.h"
#include "base/time/default_tick_clock.h"

namespace blink {
namespace scheduler {

// static
scoped_refptr<LazySchedulerMessageLoopDelegateForTests>
LazySchedulerMessageLoopDelegateForTests::Create() {
  return base::WrapRefCounted(new LazySchedulerMessageLoopDelegateForTests());
}

LazySchedulerMessageLoopDelegateForTests::
    LazySchedulerMessageLoopDelegateForTests()
    : message_loop_(base::MessageLoop::current()),
      thread_id_(base::PlatformThread::CurrentId()),
      time_source_(std::make_unique<base::DefaultTickClock>()) {
  if (message_loop_)
    original_task_runner_ = message_loop_->task_runner();
}

LazySchedulerMessageLoopDelegateForTests::
    ~LazySchedulerMessageLoopDelegateForTests() {
  RestoreDefaultTaskRunner();
}

base::MessageLoop* LazySchedulerMessageLoopDelegateForTests::EnsureMessageLoop()
    const {
  if (message_loop_)
    return message_loop_;
  DCHECK(RunsTasksInCurrentSequence());
  message_loop_ = base::MessageLoop::current();
  DCHECK(message_loop_);
  original_task_runner_ = message_loop_->task_runner();
  if (pending_task_runner_)
    message_loop_->SetTaskRunner(std::move(pending_task_runner_));
  return message_loop_;
}

void LazySchedulerMessageLoopDelegateForTests::SetDefaultTaskRunner(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  if (!HasMessageLoop()) {
    pending_task_runner_ = std::move(task_runner);
    return;
  }
  message_loop_->SetTaskRunner(std::move(task_runner));
}

void LazySchedulerMessageLoopDelegateForTests::RestoreDefaultTaskRunner() {
  if (HasMessageLoop() && base::MessageLoop::current() == message_loop_)
    message_loop_->SetTaskRunner(original_task_runner_);
}

bool LazySchedulerMessageLoopDelegateForTests::HasMessageLoop() const {
  return message_loop_ != nullptr;
}

bool LazySchedulerMessageLoopDelegateForTests::PostDelayedTask(
    const base::Location& from_here,
    base::OnceClosure task,
    base::TimeDelta delay) {
  EnsureMessageLoop();
  return original_task_runner_->PostDelayedTask(from_here, std::move(task),
                                                delay);
}

bool LazySchedulerMessageLoopDelegateForTests::PostNonNestableDelayedTask(
    const base::Location& from_here,
    base::OnceClosure task,
    base::TimeDelta delay) {
  EnsureMessageLoop();
  return original_task_runner_->PostNonNestableDelayedTask(
      from_here, std::move(task), delay);
}

bool LazySchedulerMessageLoopDelegateForTests::RunsTasksInCurrentSequence()
    const {
  return thread_id_ == base::PlatformThread::CurrentId();
}

bool LazySchedulerMessageLoopDelegateForTests::IsNested() const {
  DCHECK(RunsTasksInCurrentSequence());
  EnsureMessageLoop();
  return base::RunLoop::IsNestedOnCurrentThread();
}

void LazySchedulerMessageLoopDelegateForTests::AddNestingObserver(
    base::RunLoop::NestingObserver* observer) {
  base::RunLoop::AddNestingObserverOnCurrentThread(observer);
}

void LazySchedulerMessageLoopDelegateForTests::RemoveNestingObserver(
    base::RunLoop::NestingObserver* observer) {
  base::RunLoop::RemoveNestingObserverOnCurrentThread(observer);
}

base::TimeTicks LazySchedulerMessageLoopDelegateForTests::NowTicks() {
  return time_source_->NowTicks();
}

}  // namespace scheduler
}  // namespace blink
