// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_GLOBAL_STATE_IOS_GLOBAL_STATE_CONFIGURATION_H_
#define IOS_WEB_PUBLIC_GLOBAL_STATE_IOS_GLOBAL_STATE_CONFIGURATION_H_

#include <memory>

#include "base/task_scheduler/task_scheduler.h"

namespace ios_global_state {

// The InitParams to use for the starting of the global Task Scheduler. You may
// return null to use the default InitParams.
std::unique_ptr<base::TaskScheduler::InitParams> GetTaskSchedulerInitParams();

}  // namespace ios_global_state

#endif  // IOS_WEB_PUBLIC_GLOBAL_STATE_IOS_GLOBAL_STATE_CONFIGURATION_H_