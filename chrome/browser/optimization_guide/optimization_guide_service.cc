// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/optimization_guide/optimization_guide_service.h"

#include <string>

#include "base/bind.h"
#include "chrome/browser/browser_process.h"
#include "content/public/browser/browser_thread.h"

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
  // TODO(crbug.com/77892): Actually process hints here and dispatch indexed
  // hints to IO thread.
}

}  // namespace optimization_guide
