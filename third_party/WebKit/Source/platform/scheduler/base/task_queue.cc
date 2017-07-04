// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/base/task_queue.h"

#include "platform/scheduler/base/task_queue_impl.h"

namespace blink {
namespace scheduler {

TaskQueue::TaskQueue(internal::TaskQueueImpl* impl)
    : thread_id_(base::PlatformThread::CurrentId()), impl_(std::move(impl)) {
  impl_->SetOnQueueUnregisteredCallback(base::Bind(
      &TaskQueue::OnTaskQueueImplUnregistered, base::Unretained(this)));
}

TaskQueue::~TaskQueue() {
  if (impl_)
    impl_->SetOnQueueUnregisteredCallback(base::nullopt);
}

void TaskQueue::UnregisterTaskQueue() {
  impl_->UnregisterTaskQueue();
}

void TaskQueue::OnTaskQueueImplUnregistered(internal::TaskQueueImpl* impl) {
  DCHECK_EQ(impl, impl_);
  impl_ = nullptr;
}

bool TaskQueue::RunsTasksInCurrentSequence() const {
  return base::PlatformThread::CurrentId() == thread_id_;
}

bool TaskQueue::PostDelayedTask(const tracked_objects::Location& from_here,
                                base::OnceClosure task,
                                base::TimeDelta delay) {
  if (!impl_)
    return false;
  return impl_->PostDelayedTask(from_here, std::move(task), delay);
}

bool TaskQueue::PostNonNestableDelayedTask(
    const tracked_objects::Location& from_here,
    base::OnceClosure task,
    base::TimeDelta delay) {
  if (!impl_)
    return false;
  return impl_->PostNonNestableDelayedTask(from_here, std::move(task), delay);
}

std::unique_ptr<TaskQueue::QueueEnabledVoter>
TaskQueue::CreateQueueEnabledVoter() {
  if (!impl_)
    return nullptr;
  return impl_->CreateQueueEnabledVoter(this);
}

bool TaskQueue::IsQueueEnabled() const {
  if (!impl_)
    return false;
  return impl_->IsQueueEnabled();
}

bool TaskQueue::IsEmpty() const {
  if (!impl_)
    return true;
  return impl_->IsEmpty();
}

size_t TaskQueue::GetNumberOfPendingTasks() const {
  if (!impl_)
    return 0;
  return impl_->GetNumberOfPendingTasks();
}

bool TaskQueue::HasTaskToRunImmediately() const {
  if (!impl_)
    return false;
  return impl_->HasTaskToRunImmediately();
}

base::Optional<base::TimeTicks> TaskQueue::GetNextScheduledWakeUp() {
  if (!impl_)
    return base::nullopt;
  return impl_->GetNextScheduledWakeUp();
}

void TaskQueue::SetQueuePriority(TaskQueue::QueuePriority priority) {
  if (!impl_)
    return;
  impl_->SetQueuePriority(priority);
}

TaskQueue::QueuePriority TaskQueue::GetQueuePriority() const {
  if (!impl_)
    return TaskQueue::QueuePriority::NORMAL_PRIORITY;
  return impl_->GetQueuePriority();
}

void TaskQueue::AddTaskObserver(
    base::MessageLoop::TaskObserver* task_observer) {
  if (!impl_)
    return;
  impl_->AddTaskObserver(task_observer);
}

void TaskQueue::RemoveTaskObserver(
    base::MessageLoop::TaskObserver* task_observer) {
  if (!impl_)
    return;
  impl_->RemoveTaskObserver(task_observer);
}

void TaskQueue::SetTimeDomain(TimeDomain* time_domain) {
  if (!impl_)
    return;
  impl_->SetTimeDomain(time_domain);
}

TimeDomain* TaskQueue::GetTimeDomain() const {
  if (!impl_)
    return nullptr;
  return impl_->GetTimeDomain();
}

void TaskQueue::SetBlameContext(
    base::trace_event::BlameContext* blame_context) {
  if (!impl_)
    return;
  impl_->SetBlameContext(blame_context);
}

void TaskQueue::InsertFence(InsertFencePosition position) {
  if (!impl_)
    return;
  impl_->InsertFence(position);
}

void TaskQueue::RemoveFence() {
  if (!impl_)
    return;
  impl_->RemoveFence();
}

bool TaskQueue::HasFence() const {
  if (!impl_)
    return false;
  return impl_->HasFence();
}

bool TaskQueue::BlockedByFence() const {
  if (!impl_)
    return false;
  return impl_->BlockedByFence();
}

const char* TaskQueue::GetName() const {
  if (!impl_)
    return "";
  return impl_->GetName();
}

void TaskQueue::SetObserver(Observer* observer) {
  if (!impl_)
    return;
  if (observer) {
    // Observer is guaranteed to outlive TaskQueue and TaskQueueImpl lifecycle
    // is controlled by |this|.
    impl_->SetOnNextWakeUpChangedCallback(
        base::Bind(&TaskQueue::Observer::OnQueueNextWakeUpChanged,
                   base::Unretained(observer), base::Unretained(this)));
  } else {
    impl_->SetOnNextWakeUpChangedCallback(base::nullopt);
  }
}

internal::TaskQueueImpl* TaskQueue::GetTaskQueueImpl() const {
  return impl_;
}

internal::TaskQueueImpl* TaskQueue::GetTaskQueueImplForTest() const {
  return impl_;
}

}  // namespace scheduler
}  // namespace blink
