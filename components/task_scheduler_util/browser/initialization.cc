// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/task_scheduler_util/browser/initialization.h"

#include <map>
#include <string>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/task_scheduler/switches.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/platform_thread.h"
#include "base/threading/sequenced_worker_pool.h"
#include "components/task_scheduler_util/common/variations_util.h"
#include "components/variations/variations_associated_data.h"

namespace task_scheduler_util {

namespace {

constexpr char kFieldTrialName[] = "BrowserScheduler";

enum WorkerPoolType : size_t {
  BACKGROUND = 0,
  BACKGROUND_FILE_IO,
  FOREGROUND,
  FOREGROUND_FILE_IO,
  WORKER_POOL_COUNT  // Always last.
};

}  // namespace

std::vector<base::SchedulerWorkerPoolParams>
GetBrowserWorkerPoolParamsFromVariations() {
  using ThreadPriority = base::ThreadPriority;

  std::map<std::string, std::string> variation_params;
  if (!::variations::GetVariationParams(kFieldTrialName, &variation_params))
    return std::vector<base::SchedulerWorkerPoolParams>();

  std::vector<SchedulerImmutableWorkerPoolParams> constant_worker_pool_params;
  DCHECK_EQ(BACKGROUND, constant_worker_pool_params.size());
  constant_worker_pool_params.emplace_back("Background",
                                           ThreadPriority::BACKGROUND);
  DCHECK_EQ(BACKGROUND_FILE_IO, constant_worker_pool_params.size());
  constant_worker_pool_params.emplace_back("BackgroundFileIO",
                                           ThreadPriority::BACKGROUND);
  DCHECK_EQ(FOREGROUND, constant_worker_pool_params.size());
  constant_worker_pool_params.emplace_back("Foreground",
                                           ThreadPriority::NORMAL);
  DCHECK_EQ(FOREGROUND_FILE_IO, constant_worker_pool_params.size());
  constant_worker_pool_params.emplace_back("ForegroundFileIO",
                                           ThreadPriority::NORMAL);

  return GetWorkerPoolParams(constant_worker_pool_params, variation_params);
}

size_t BrowserWorkerPoolIndexForTraits(const base::TaskTraits& traits) {
  const bool is_background =
      traits.priority() == base::TaskPriority::BACKGROUND;
  if (traits.with_file_io())
    return is_background ? BACKGROUND_FILE_IO : FOREGROUND_FILE_IO;
  return is_background ? BACKGROUND : FOREGROUND;
}

void MaybePerformBrowserTaskSchedulerRedirection() {
  // TODO(gab): Remove this when http://crbug.com/622400 concludes.
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableBrowserTaskScheduler) &&
      variations::GetVariationParamValue(
          kFieldTrialName, "RedirectSequencedWorkerPools") == "true") {
    const base::TaskPriority max_task_priority =
        variations::GetVariationParamValue(
            kFieldTrialName, "CapSequencedWorkerPoolsAtUserVisible") == "true"
            ? base::TaskPriority::USER_VISIBLE
            : base::TaskPriority::HIGHEST;
    base::SequencedWorkerPool::EnableWithRedirectionToTaskSchedulerForProcess(
        max_task_priority);
  }
}

}  // namespace task_scheduler_util
