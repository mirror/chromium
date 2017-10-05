// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/child/web_task_runner_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "platform/scheduler/base/task_queue.h"
#include "platform/scheduler/base/time_domain.h"
#include "public/platform/WebTraceLocation.h"

namespace blink {
namespace scheduler {

namespace {

void RunCrossThreadClosure(CrossThreadClosure task) {
  task();
}

}  // namespace

RefPtr<WebTaskRunnerImpl> WebTaskRunnerImpl::Create(
    scoped_refptr<TaskQueue> task_queue) {
  return WTF::AdoptRef(new WebTaskRunnerImpl(std::move(task_queue)));
}

bool WebTaskRunnerImpl::RunsTasksInCurrentSequence() {
  return task_queue_->RunsTasksInCurrentSequence();
}

double WebTaskRunnerImpl::VirtualTimeSeconds() const {
  return (Now() - base::TimeTicks::UnixEpoch()).InSecondsF();
}

double WebTaskRunnerImpl::MonotonicallyIncreasingVirtualTimeSeconds() const {
  return Now().ToInternalValue() /
         static_cast<double>(base::Time::kMicrosecondsPerSecond);
}

WebTaskRunnerImpl::WebTaskRunnerImpl(scoped_refptr<TaskQueue> task_queue)
    : task_queue_(std::move(task_queue)) {}

WebTaskRunnerImpl::~WebTaskRunnerImpl() {}

base::TimeTicks WebTaskRunnerImpl::Now() const {
  TimeDomain* time_domain = task_queue_->GetTimeDomain();
  // It's possible task_queue_ has been Unregistered which can lead to a null
  // TimeDomain.  If that happens just return the current real time.
  if (!time_domain)
    return base::TimeTicks::Now();
  return time_domain->Now();
}

scoped_refptr<base::SingleThreadTaskRunner>
WebTaskRunnerImpl::ToSingleThreadTaskRunner() {
  return task_queue_.get();
}

// Use a custom function for base::Bind instead of convertToBaseCallback to
// avoid copying the closure later in the call chain. Copying the bound state
// can lead to data races with ref counted objects like StringImpl. See
// crbug.com/679915 for more details.
void WebTaskRunnerImpl::PostTask(const WebTraceLocation& location,
                                 CrossThreadClosure task) {
  task_queue_->PostTaskWithMetadata(TaskQueue::PostedTask(
      base::BindOnce(&RunCrossThreadClosure, std::move(task)), location,
      base::TimeDelta(), false));
}

void WebTaskRunnerImpl::PostDelayedTask(const WebTraceLocation& location,
                                        CrossThreadClosure task,
                                        TimeDelta delay) {
  task_queue_->PostTaskWithMetadata(TaskQueue::PostedTask(
      base::BindOnce(&RunCrossThreadClosure, std::move(task)), location, delay,
      false));
}

void WebTaskRunnerImpl::PostTask(const WebTraceLocation& location,
                                 WTF::Closure task) {
  task_queue_->PostTaskWithMetadata(
      TaskQueue::PostedTask(ConvertToBaseCallback(std::move(task)), location,
                            base::TimeDelta(), false));
}

void WebTaskRunnerImpl::PostDelayedTask(const WebTraceLocation& location,
                                        WTF::Closure task,
                                        TimeDelta delay) {
  task_queue_->PostTaskWithMetadata(TaskQueue::PostedTask(
      ConvertToBaseCallback(std::move(task)), location, delay, false));
}

}  // namespace scheduler
}  // namespace blink
