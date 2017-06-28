// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/net/global_state/ios_global_state.h"

#include "base/memory/ptr_util.h"
#include "base/task_scheduler/initialization_util.h"
#include "components/task_scheduler_util/browser/initialization.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

std::unique_ptr<base::TaskScheduler::InitParams>
GetDefaultTaskSchedulerInitParams() {
  using StandbyThreadPolicy =
      base::SchedulerWorkerPoolParams::StandbyThreadPolicy;
  return base::MakeUnique<base::TaskScheduler::InitParams>(
      base::SchedulerWorkerPoolParams(
          StandbyThreadPolicy::ONE,
          base::RecommendedMaxNumberOfThreadsInPool(2, 8, 0.1, 0),
          base::TimeDelta::FromSeconds(30)),
      base::SchedulerWorkerPoolParams(
          StandbyThreadPolicy::ONE,
          base::RecommendedMaxNumberOfThreadsInPool(2, 8, 0.1, 0),
          base::TimeDelta::FromSeconds(30)),
      base::SchedulerWorkerPoolParams(
          StandbyThreadPolicy::ONE,
          base::RecommendedMaxNumberOfThreadsInPool(3, 8, 0.3, 0),
          base::TimeDelta::FromSeconds(30)),
      base::SchedulerWorkerPoolParams(
          StandbyThreadPolicy::ONE,
          base::RecommendedMaxNumberOfThreadsInPool(3, 8, 0.3, 0),
          base::TimeDelta::FromSeconds(60)));
}

}  // namespace

namespace global_state {

void Create() {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
#if CRONET_BUILD
    base::TaskScheduler::Create("CronetIos");
#else
    // Use an empty string as TaskScheduler name to match the suffix of browser
    // process TaskScheduler histograms.
    base::TaskScheduler::Create("");
#endif
  });
}

void StartTaskScheduler() {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    auto init_params_from_variations =
        task_scheduler_util::GetBrowserTaskSchedulerInitParamsFromVariations();

    if (init_params_from_variations) {
      base::TaskScheduler::GetInstance()->Start(*init_params_from_variations);
    } else {
      auto task_scheduler_init_params = GetDefaultTaskSchedulerInitParams();
      DCHECK(task_scheduler_init_params);
      base::TaskScheduler::GetInstance()->Start(
          *task_scheduler_init_params.get());
    }
  });
}

}  // namespace global_state
