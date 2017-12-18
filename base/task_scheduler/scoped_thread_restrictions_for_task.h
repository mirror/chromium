// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_SCHEDULER_SCOPED_THREAD_RESTRICTIONS_FOR_TASK_H_
#define BASE_TASK_SCHEDULER_SCOPED_THREAD_RESTRICTIONS_FOR_TASK_H_

#include "base/task_scheduler/task.h"

namespace base {
namespace internal {

class ScopedThreadRestrictionsForTask {
 public:
  ScopedThreadRestrictionsForTask(const Task& task);
  ~ScopedThreadRestrictionsForTask();

 private:
  const bool previous_singleton_allowed_;
  const bool previous_io_allowed_;
  const bool previous_wait_allowed_;

  DISALLOW_COPY_AND_ASSIGN(ScopedThreadRestrictionsForTask);
};

}  // namespace internal
}  // namespace base

#endif  // BASE_TASK_SCHEDULER_SCOPED_THREAD_RESTRICTIONS_FOR_TASK_H_
