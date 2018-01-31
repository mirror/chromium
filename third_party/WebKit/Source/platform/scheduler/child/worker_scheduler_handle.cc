// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/child/worker_scheduler_handle.h"

namespace blink {
namespace scheduler {

WorkerSchedulerHandle::WorkerSchedulerHandle(
    scoped_refptr<internal::WorkerSchedulerState> proxy)
    : proxy_(proxy) {}

WorkerSchedulerHandle::~WorkerSchedulerHandle() {
  proxy_->OnHandleDestroyed();
}

scoped_refptr<internal::WorkerSchedulerState> WorkerSchedulerHandle::Get() {
  return proxy_;
}

}  // namespace scheduler
}  // namespace blink
