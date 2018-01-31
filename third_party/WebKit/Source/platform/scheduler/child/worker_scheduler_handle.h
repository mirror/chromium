// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_CHILD_WORKER_SCHEDULER_PROXY_HANDLE_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_CHILD_WORKER_SCHEDULER_PROXY_HANDLE_H_

#include "base/macros.h"
// TODO(altimin): WorkerSchedulerState can't be forward-declared due to
// being refcounted. Consider changing that.
#include "platform/PlatformExport.h"
#include "platform/scheduler/child/worker_scheduler_state.h"

namespace blink {
namespace scheduler {

// Public class exposing a handle to WorkerSchedulerState to be retained by
// DedicatedWorker.
class PLATFORM_EXPORT WorkerSchedulerHandle {
 public:
  explicit WorkerSchedulerHandle(
      scoped_refptr<internal::WorkerSchedulerState> proxy);
  ~WorkerSchedulerHandle();

  scoped_refptr<internal::WorkerSchedulerState> Get();

 private:
  scoped_refptr<internal::WorkerSchedulerState> proxy_;

  DISALLOW_COPY_AND_ASSIGN(WorkerSchedulerHandle);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_CHILD_WORKER_SCHEDULER_PROXY_HANDLE_H_
