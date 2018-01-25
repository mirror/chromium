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
// - Implementation of the worker obtaines the handle to this proxy by
//   calling WebFrameSchedulerImpl::CreateWorkerSchedulerProxyHandle.
// - WebFrameSchedulerImpl retains a scoped_refptr to WorkerSchedulerProxy and
//   uses it pass signals to WorkerScheduler via the proxy.
// - WorkerSchedulerProxyHandle is owned by the implementation of the worker.
//   Destroying the handle initiates the shutdown of the proxy.
// - A raw pointer to WorkerSchedulerProxyHandle is passed to WebThread, which
//   obtains WorkerSchedulerProxy and uses it to initialize WorkerScheduler.
class PLATFORM_EXPORT WorkerSchedulerProxy
    : public ThreadSafeRefCounted<WorkerSchedulerProxy> {
 public:
  // Called on the main thread.
  explicit WorkerSchedulerProxy(WebFrameSchedulerImpl* frame_scheduler);
  ~WorkerSchedulerProxy();

  // Should be called on the worker thread.
  void OnWorkerSchedulerCreated(WorkerSchedulerImpl* worker_scheduler);

  // Notify worker scheduler that the associated page became invisible.
  // Should be called on the main thread.
  void OnPageVisible(bool visible);

  // Should be called on the main thread.
  void OnParentObjectDestroyed();

  // Should be called on the worker thread.
  void OnWorkerSchedulerDestroyed();

  bool initial_page_visibility() const { return initial_page_visibility_; }

 private:
  // Can be accessed only on the main thread.
  base::WeakPtr<WebFrameSchedulerImpl> frame_scheduler_;
  // Can be accessed only on the worker thread.
  base::WeakPtr<WorkerSchedulerImpl> worker_scheduler_;

  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> worker_thread_task_runner_;

  bool destroyed_on_main_thread_ = false;
  bool destroyed_on_worker_thread_ = false;

  base::PlatformThreadRef main_thread_ref_;
  base::PlatformThreadRef worker_thread_ref_;

  // The visibility of the page when the proxy was created.
  const bool initial_page_visibility_;

  DISALLOW_COPY_AND_ASSIGN(WorkerSchedulerProxy);
};

}  // namespace internal
}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_CHILD_WORKER_SCHEDULER_PROXY_H_
