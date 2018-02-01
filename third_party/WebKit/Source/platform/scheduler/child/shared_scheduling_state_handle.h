// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_CHILD_WORKER_SCHEDULER_PROXY_HANDLE_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_CHILD_WORKER_SCHEDULER_PROXY_HANDLE_H_

#include "base/macros.h"
// TODO(altimin): SharedSchedulingState can't be forward-declared due to
// being refcounted. Consider changing that.
#include "platform/PlatformExport.h"
#include "platform/scheduler/child/shared_scheduling_state.h"

namespace blink {
namespace scheduler {

// Public class exposing a handle to SharedSchedulingState to be retained by
// the owner of the thread.
// (currently the only owner of this handle is DedicatedWorker)
class PLATFORM_EXPORT SharedSchedulingStateHandle {
 public:
  explicit SharedSchedulingStateHandle(
      scoped_refptr<internal::SharedSchedulingState> proxy);
  ~SharedSchedulingStateHandle();

  scoped_refptr<internal::SharedSchedulingState> Get();

 private:
  scoped_refptr<internal::SharedSchedulingState> state_;

  DISALLOW_COPY_AND_ASSIGN(SharedSchedulingStateHandle);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_CHILD_WORKER_SCHEDULER_PROXY_HANDLE_H_
