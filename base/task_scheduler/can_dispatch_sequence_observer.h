// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_SCHEDULER_CAN_DISPATCH_SEQUENCE_OBSERVER_H_
#define BASE_TASK_SCHEDULER_CAN_DISPATCH_SEQUENCE_OBSERVER_H_

#include "base/task_scheduler/sequence.h"

namespace base {
namespace internal {

class CanDispatchSequenceObserver {
 public:
  // Called when |sequence| can be dispatched. It is expected that
  // TaskTracker::RunNextTask() will be called with |sequence| as argument after
  // this is called.
  virtual void OnCanDispatchSequence(scoped_refptr<Sequence> sequence) = 0;

 protected:
  virtual ~CanDispatchSequenceObserver() = default;
};

}  // namespace internal
}  // namespace base

#endif  // BASE_TASK_SCHEDULER_CAN_DISPATCH_SEQUENCE_OBSERVER_H_
