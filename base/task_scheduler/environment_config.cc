// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/environment_config.h"

namespace base {
namespace internal {

bool SchedulerHasBackgroundEnvironment() {
  // When Lock doesn't handle multiple thread priorities, all threads have a
  // NORMAL priority. Otherwise, priority inversion could occur.
  //
  // When the priority of a thread can't be increased, all threads have a NORMAL
  // priority. Otherwise, it wouldn't be possible to increase the priority of
  // BACKGROUND threads during shutdown to prevent hangs.
  return Lock::HandlesMultipleThreadPriorities() &&
         PlatformThread::CanIncreaseCurrentThreadPriority();
}

size_t GetEnvironmentIndexForTraits(const TaskTraits& traits) {
  const bool use_background_environment =
      traits.priority() == base::TaskPriority::BACKGROUND &&
      SchedulerHasBackgroundEnvironment();
  if (traits.may_block() || traits.with_base_sync_primitives())
    return use_background_environment ? BACKGROUND_BLOCKING
                                      : FOREGROUND_BLOCKING;
  return use_background_environment ? BACKGROUND : FOREGROUND;
}

}  // namespace internal
}  // namespace base
