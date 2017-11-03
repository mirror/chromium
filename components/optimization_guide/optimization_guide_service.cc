// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/optimization_guide/optimization_guide_service.h"

#include <string>

#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/task_scheduler/post_task.h"
#include "components/optimization_guide/notification_types.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"

namespace optimization_guide {

namespace {

// Version "0" corresponds to no processed version. By service conventions,
// we represent it as a dotted triple.
const char kNullVersion[] = "0.0.0";

}  // namespace

UnindexedHintsInfo::UnindexedHintsInfo(const base::Version& hints_version,
                                       const base::FilePath& hints_path)
    : hints_version(hints_version), hints_path(hints_path) {}

UnindexedHintsInfo::~UnindexedHintsInfo() {}

OptimizationGuideService::OptimizationGuideService(
    const scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner)
    : background_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND})),
      io_thread_task_runner_(io_thread_task_runner),
      latest_processed_version_(kNullVersion) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

OptimizationGuideService::~OptimizationGuideService() {}

void OptimizationGuideService::SetLatestProcessedVersionForTesting(
    base::Version version) {
  latest_processed_version_ = version;
}

void OptimizationGuideService::ReadAndIndexHints(
    const UnindexedHintsInfo& unindexed_hints_info) {
  background_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&OptimizationGuideService::ReadAndIndexHintsInBackground,
                 base::Unretained(this), unindexed_hints_info));
}

void OptimizationGuideService::ReadAndIndexHintsInBackground(
    const UnindexedHintsInfo& unindexed_hints_info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!unindexed_hints_info.hints_version.IsValid())
    return;
  if (latest_processed_version_.CompareTo(unindexed_hints_info.hints_version) >=
      0)
    return;
  if (unindexed_hints_info.hints_path.empty())
    return;
  std::string binary_pb;
  if (!base::ReadFileToString(unindexed_hints_info.hints_path, &binary_pb))
    return;

  std::unique_ptr<Configuration> new_config = std::make_unique<Configuration>();
  if (!new_config->ParseFromString(binary_pb)) {
    DVLOG(1) << "Failed parsing proto";
    return;
  }
  latest_processed_version_ = unindexed_hints_info.hints_version;
  io_thread_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&OptimizationGuideService::DispatchHintsOnIOThread,
                 base::Unretained(this), base::Passed(std::move(new_config))));
}

void OptimizationGuideService::DispatchHintsOnIOThread(
    std::unique_ptr<Configuration> config) {
  DCHECK(io_thread_task_runner_->BelongsToCurrentThread());

  content::NotificationService::current()->Notify(
      NOTIFICATION_HINTS_READY, content::Source<OptimizationGuideService>(this),
      content::Details<Configuration>(config.release()));
}

}  // namespace optimization_guide
