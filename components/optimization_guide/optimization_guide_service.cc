// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/optimization_guide/optimization_guide_service.h"

#include <string>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "components/optimization_guide/notification_types.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"

using content::BrowserThread;

namespace optimization_guide {

UnindexedHintsInfo::UnindexedHintsInfo(const base::Version& hints_version,
                                       const base::FilePath& hints_path)
    : hints_version(hints_version), hints_path(hints_path) {}

UnindexedHintsInfo::~UnindexedHintsInfo() {}

OptimizationGuideService::OptimizationGuideService(
    const scoped_refptr<base::SequencedTaskRunner> background_task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner)
    : background_task_runner_(background_task_runner),
      io_thread_task_runner_(io_thread_task_runner) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

OptimizationGuideService::~OptimizationGuideService() {}

void OptimizationGuideService::ReadAndIndexHints(
    const UnindexedHintsInfo& unindexed_hints_info) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  background_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&OptimizationGuideService::ReadAndIndexHintsInBackground,
                 base::Unretained(this), unindexed_hints_info));
}

void OptimizationGuideService::ReadAndIndexHintsInBackground(
    const UnindexedHintsInfo& unindexed_hints_info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!unindexed_hints_info.hints_version.IsValid()) {
    return;
  }
  if (unindexed_hints_info.hints_path.empty()) {
    return;
  }
  std::string binary_pb;
  if (!base::ReadFileToString(unindexed_hints_info.hints_path, &binary_pb)) {
    return;
  }
  Configuration new_config;
  if (!new_config.ParseFromString(binary_pb)) {
    DVLOG(1) << "Failed parsing proto";
    return;
  }
  io_thread_task_runner_->PostTask(
      FROM_HERE, base::Bind(&OptimizationGuideService::DispatchOnIOThread,
                            base::Unretained(this), new_config));
}

void OptimizationGuideService::DispatchOnIOThread(
    const Configuration& configuration) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  content::NotificationService::current()->Notify(
      NOTIFICATION_HINTS_PROCESSED,
      content::Source<OptimizationGuideService>(this),
      content::Details<const Configuration>(&configuration));
}

}  // namespace optimization_guide
