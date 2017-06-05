// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/lazy_task_runner.h"

#include <type_traits>

#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/scheduler_lock.h"

namespace base {
namespace internal {
/*
template <>
scoped_refptr<SequencedTaskRunner> LazyTaskRunner<SequencedTaskRunner>::Get() {
  return nullptr;
}*/

}  // namespace internal
}  // namespace base
