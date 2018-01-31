// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_CHILD_WORKER_SCHEDULER_PROXY_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_CHILD_WORKER_SCHEDULER_PROXY_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "platform/PlatformExport.h"
#include "platform/scheduler/child/page_visibility_state.h"
#include "platform/wtf/ThreadSafeRefCounted.h"

namespace blink {
namespace scheduler {
class WebFrameSchedulerImpl;
class WorkerSchedulerImpl;

namespace internal {

// Thread-safe helper class for communication between frame scheduler and
// worker scheduler.
//
// It is owned cooperatively by WebFrameSchedulerImpl and WorkerSchedulerImpl.
// The initialisation sequence works as follows:
// - Implementation of the worker obtains the handle to this proxy by
//   calling WebFrameSchedulerImpl::CreateWorkerSchedulerHandle.
// - WebFrameSchedulerImpl retains a scoped_refptr to WorkerSchedulerState and
//   uses it to pass signals to WorkerScheduler via the proxy.
// - WorkerSchedulerHandle is owned by the implementation of the worker.
//   Destroying the handle initiates the shutdown of the proxy.
//   (TODO(altimin): Implement the item above).
// - A raw pointer to WorkerSchedulerHandle is passed to WebThread, which
//   obtains WorkerSchedulerState and uses it to initialize WorkerScheduler.
class PLATFORM_EXPORT WorkerSchedulerState
    : public ThreadSafeRefCounted<WorkerSchedulerState> {
 public:
  // Called on the main thread.
  explicit WorkerSchedulerState(WebFrameSchedulerImpl* frame_scheduler);
  ~WorkerSchedulerState();

  // Should be called on the worker thread.
  void OnWorkerSchedulerCreated(WorkerSchedulerImpl* worker_scheduler);

  // Notify worker scheduler that the associated page became visible or
  // invisible.
  // Should be called on the main thread.
  void OnPageVisibilityStateChanged(PageVisibilityState visibility);

  // Should be called on the main thread.
  void OnHandleDestroyed();

  // Should be called on the worker thread.
  void OnWorkerSchedulerDestroyed();

  PageVisibilityState initial_page_visibility() const {
    return initial_page_visibility_;
  }

 private:
  bool IsMainThread() const;
  bool IsWorkerThread() const;

  // Can be accessed only on the main thread.
  base::WeakPtr<WebFrameSchedulerImpl> frame_scheduler_;
  bool destroyed_on_main_thread_ = false;
  // Can be accessed only on the worker thread.
  base::WeakPtr<WorkerSchedulerImpl> worker_scheduler_;
  bool destroyed_on_worker_thread_ = false;

  // These variables are initialized in the constructor and
  // in OnWorkerSchedulerCreated. Given that they stay const after that
  // and main thread is blocked while worker thread is being initialized,
  // they are safe to use from any thread.
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> worker_thread_task_runner_;
  base::PlatformThreadRef main_thread_ref_;
  base::PlatformThreadRef worker_thread_ref_;

  // The visibility of the page when the proxy was created.
  const PageVisibilityState initial_page_visibility_;

  DISALLOW_COPY_AND_ASSIGN(WorkerSchedulerState);
};

}  // namespace internal
}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_CHILD_WORKER_SCHEDULER_PROXY_H_
