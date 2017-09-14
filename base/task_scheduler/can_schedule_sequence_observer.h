// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_SCHEDULER_CAN_SCHEDULE_SEQUENCE_OBSERVER_H_
#define BASE_TASK_SCHEDULER_CAN_SCHEDULE_SEQUENCE_OBSERVER_H_

#include "base/task_scheduler/sequence.h"

namespace base {
namespace internal {

class CanScheduleSequenceObserver {
 public:
  virtual ~CanScheduleSequenceObserver() {}

  // Called when |sequence| can be scheduled. Shortly after this is called,
  // TaskTracker::RunNextTask() must be called with |sequence| as argument.
  virtual void OnCanScheduleSequence(scoped_refptr<Sequence> sequence) = 0;
};

}  // namespace internal
}  // namespace base

#endif  // BASE_TASK_SCHEDULER_CAN_SCHEDULE_SEQUENCE_OBSERVER_H_
