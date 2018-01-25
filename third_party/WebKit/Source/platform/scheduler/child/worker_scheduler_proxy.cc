// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/child/worker_scheduler_proxy.h"

#include "platform/scheduler/child/worker_scheduler_impl.h"
#include "platform/scheduler/renderer/web_frame_scheduler_impl.h"

namespace blink {
namespace scheduler {
namespace internal {

WorkerSchedulerProxy::WorkerSchedulerProxy(
    WebFrameSchedulerImpl* frame_scheduler)
    : frame_scheduler_(frame_scheduler->GetWeakPtr()),
      main_thread_task_runner_(frame_scheduler->ControlTaskQueue()),
      main_thread_ref_(base::PlatformThread::CurrentRef()),
      initial_page_visibility_(frame_scheduler->IsPageVisible()) {}

WorkerSchedulerProxy::~WorkerSchedulerProxy() {
  // Due to having weak pointers ensure that they are cleared both on
  // the main thread and the worker thread.
  DCHECK(destroyed_on_main_thread_);
  DCHECK(destroyed_on_worker_thread_);
}

void WorkerSchedulerProxy::OnWorkerSchedulerCreated(
    WorkerSchedulerImpl* worker_scheduler) {
  DCHECK(main_thread_ref_ != base::PlatformThread::CurrentRef())
      << "OnWorkerSchedulerCreated should be called from the worker thread";
  DCHECK(!worker_scheduler_) << "OnWorkerSchedulerCreated is called twice";
  worker_scheduler_ = worker_scheduler->GetWeakPtr();
  worker_thread_task_runner_ = worker_scheduler->ControlTaskQueue();
  worker_thread_ref_ = base::PlatformThread::CurrentRef();
}

void WorkerSchedulerProxy::OnPageVisible(bool visible) {
  DCHECK(main_thread_ref_ == base::PlatformThread::CurrentRef());
  worker_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&WorkerSchedulerImpl::OnPageVisible,
                                worker_scheduler_, visible));
}

void WorkerSchedulerProxy::OnParentObjectDestroyed() {
  DCHECK(main_thread_ref_ == base::PlatformThread::CurrentRef());

  if (destroyed_on_main_thread_)
    return;

  destroyed_on_main_thread_ = true;

  if (!frame_scheduler_)
    return;

  // Retain a reference for the duration of this function to ensure that
  // this object stays alive during this function.
  scoped_refptr<WorkerSchedulerProxy> ref(this);

  frame_scheduler_->UnregisterWorkerSchedulerProxy(ref);
  frame_scheduler_ = nullptr;
}

void WorkerSchedulerProxy::OnWorkerSchedulerDestroyed() {
  DCHECK(worker_thread_ref_ == base::PlatformThread::CurrentRef());

  destroyed_on_worker_thread_ = true;
  worker_scheduler_ = nullptr;
}

}  // namespace internal
}  // namespace scheduler
}  // namespace blink
