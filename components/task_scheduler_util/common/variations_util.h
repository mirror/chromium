// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_TASK_SCHEDULER_UTIL_COMMON_VARIATIONS_UTIL_H_
#define COMPONENTS_TASK_SCHEDULER_UTIL_COMMON_VARIATIONS_UTIL_H_

#include <memory>

#include "base/strings/string_piece.h"
#include "base/task_scheduler/task_scheduler.h"

namespace task_scheduler_util {

// Builds a TaskScheduler::InitParams from pool descriptors in
// |variations_params| that are prefixed with |variation_param_prefix|. Returns
// nullptr on failure.
std::unique_ptr<base::TaskScheduler::InitParams> GetTaskSchedulerInitParams(
    base::StringPiece variation_param_prefix);

}  // namespace task_scheduler_util

#endif  // COMPONENTS_TASK_SCHEDULER_UTIL_COMMON_VARIATIONS_UTIL_H_
