// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

namespace base {
namespace internal {

// Interface to inform a SchedulerWorkerPool of tasks that become blocked or
// unblocked.
class BASE_EXPORT BlockingObserver {
 public:
  virtual void TaskBlocked() = 0;
  virtual void TaskUnblocked() = 0;
};

}  // namespace internal
}  // namespace base
