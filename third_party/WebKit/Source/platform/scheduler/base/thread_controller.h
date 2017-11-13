// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_WORKER_CONTROLLER_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_WORKER_CONTROLLER_H_

#include "base/location.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/time/time.h"
#include "platform/PlatformExport.h"

namespace base {
class TickClock;
};

namespace blink {
namespace scheduler {
namespace internal {

class Sequence;

// Interface for TaskQueueManager to schedule work to be run.
class PLATFORM_EXPORT ThreadController {
 public:
  virtual ~ThreadController() {}

  virtual void ScheduleWork() = 0;

  virtual void ScheduleDelayedWork(base::TimeDelta delay) = 0;

  virtual void CancelDelayedWork() = 0;

  virtual void SetSequence(Sequence*) = 0;

  // TODO(altimin, crbug.com/783309): Get rid of the methods below.
  virtual void PostNonNestableTask(const base::Location& from_here,
                                   base::Closure task) = 0;

  virtual bool RunsTasksInCurrentSequence() = 0;

  virtual base::TickClock* GetClock() = 0;

  virtual void SetDefaultTaskRunner(
      scoped_refptr<base::SingleThreadTaskRunner>) = 0;

  virtual void RestoreDefaultTaskRunner() = 0;

  virtual bool IsNested() = 0;

  virtual void AddNestingObserver(base::RunLoop::NestingObserver* observer) = 0;

  virtual void RemoveNestingObserver(
      base::RunLoop::NestingObserver* observer) = 0;
};

}  // namespace internal
}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_WORKER_CONTROLLER_H_
