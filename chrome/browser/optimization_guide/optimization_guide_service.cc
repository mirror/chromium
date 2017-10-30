// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/optimization_guide/optimization_guide_service.h"

#include <string>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/io_thread.h"
#include "chrome/browser/optimization_guide/proto/hints.pb.h"
#include "components/previews/core/previews_optimization_guide.h"
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

  previews::PreviewsOptimizationGuide* previews_optimization_guide =
      g_browser_process->io_thread()->previews_optimization_guide();
  background_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&OptimizationGuideService::ReadAndIndexHintsInBackground,
                 base::Unretained(this), unindexed_hints_info,
                 previews_optimization_guide));
}

void OptimizationGuideService::ReadAndIndexHintsInBackground(
    const UnindexedHintsInfo& unindexed_hints_info,
    previews::PreviewsOptimizationGuide* previews_optimization_guide) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!unindexed_hints_info.hints_version.IsValid()) {
    return;
  }
  if (unindexed_hints_info.hints_path.empty()) {
    return;
  }
  // TODO(crbug.com/77892): Do something with hints here.
  previews::Hints hints;
  io_thread_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&OptimizationGuideService::UpdateHintsOnIOThread,
                 base::Unretained(this), hints, previews_optimization_guide));
}

void OptimizationGuideService::UpdateHintsOnIOThread(
    const previews::Hints& hints,
    previews::PreviewsOptimizationGuide* previews_optimization_guide) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  previews_optimization_guide->SwapHints(hints);
}

}  // namespace optimization_guide
