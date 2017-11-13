// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_TASK_QUEUE_MANAGER_DELEGATE_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_TASK_QUEUE_MANAGER_DELEGATE_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "base/time/tick_clock.h"
#include "platform/PlatformExport.h"

namespace blink {
namespace scheduler {

class PLATFORM_EXPORT TaskQueueManagerDelegate
    : public base::SingleThreadTaskRunner,
      public base::TickClock {
 public:
  TaskQueueManagerDelegate() {}

 protected:
  ~TaskQueueManagerDelegate() override {}

  DISALLOW_COPY_AND_ASSIGN(TaskQueueManagerDelegate);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_TASK_QUEUE_MANAGER_DELEGATE_H_
