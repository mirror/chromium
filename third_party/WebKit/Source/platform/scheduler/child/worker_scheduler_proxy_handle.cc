// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/child/worker_scheduler_proxy_handle.h"

namespace blink {
namespace scheduler {

WorkerSchedulerProxyHandle::WorkerSchedulerProxyHandle(
    scoped_refptr<internal::WorkerSchedulerProxy> proxy)
    : proxy_(proxy) {}

WorkerSchedulerProxyHandle::~WorkerSchedulerProxyHandle() {
  proxy_->OnParentObjectDestroyed();
}

scoped_refptr<internal::WorkerSchedulerProxy>
WorkerSchedulerProxyHandle::GetWorkerSchedulerProxy() {
  return proxy_;
}

}  // namespace scheduler
}  // namespace blink
