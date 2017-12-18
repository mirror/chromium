// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/scoped_thread_restrictions_for_task.h"

#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread_restrictions.h"

namespace base {
namespace internal {

ScopedThreadRestrictionsForTask::ScopedThreadRestrictionsForTask(
    const Task& task)
    : previous_singleton_allowed_(ThreadRestrictions::SetSingletonAllowed(
          task.traits.shutdown_behavior() !=
          TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN)),
      previous_io_allowed_(
          ThreadRestrictions::SetIOAllowed(task.traits.may_block())),
      previous_wait_allowed_(ThreadRestrictions::SetWaitAllowed(
          task.traits.with_base_sync_primitives())) {}

ScopedThreadRestrictionsForTask::~ScopedThreadRestrictionsForTask() {
  ThreadRestrictions::SetWaitAllowed(previous_wait_allowed_);
  ThreadRestrictions::SetIOAllowed(previous_io_allowed_);
  ThreadRestrictions::SetSingletonAllowed(previous_singleton_allowed_);
}

}  // namespace internal
}  // namespace base
