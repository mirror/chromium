// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef BASE_TASK_SCHEDULER_BLOCKING_OBSERVER_H_
#define BASE_TASK_SCHEDULER_BLOCKING_OBSERVER_H_

#include "base/base_export.h"

namespace base {
namespace internal {

// Interface to inform a SchedulerWorkerPool of tasks that become blocked or
// unblocked.
class BASE_EXPORT BlockingObserver {
 public:
  virtual void TaskBlocked() = 0;
  virtual void TaskUnblocked() = 0;
};

BASE_EXPORT BlockingObserver* GetBlockingObserver();

void SetBlockingObserver(BlockingObserver* blocking_observer);

}  // namespace internal
}  // namespace base

#endif  // BASE_TASK_SCHEDULER_BLOCKING_OBSERVER_H_
