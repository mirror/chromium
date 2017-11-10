// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_SEQUENCE_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_SEQUENCE_H_

namespace blink {
namespace scheduler {
namespace internal {

// This is temporary interface for ThreadController to be able to run tasks
// from TaskQueueManager.
class Sequence {
 public:
  virtual base::PendingTask TakeTask(bool delayed) = 0;

  virtual void DidRunTask() = 0;
};

}  // namespace internal
}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_SEQUENCE_H_
