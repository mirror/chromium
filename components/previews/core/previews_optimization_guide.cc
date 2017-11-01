// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/previews_optimization_guide.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "components/optimization_guide/notification_types.h"
#include "components/optimization_guide/optimization_guide_service.h"
#include "components/optimization_guide/proto/hints.pb.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "net/url_request/url_request.h"

namespace previews {

Hints::Hints() {}

Hints::Hints(const Hints& other) : host_suffixes(other.host_suffixes) {}

Hints::~Hints() {}

void Hints::AddHint(const std::string& host_suffix) {
  host_suffixes.push_back(host_suffix);
}

PreviewsOptimizationGuide::PreviewsOptimizationGuide(
    const scoped_refptr<base::SequencedTaskRunner>& background_task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
    optimization_guide::OptimizationGuideService* optimization_guide_service)
    : background_task_runner_(background_task_runner),
      io_task_runner_(io_task_runner) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
  io_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&PreviewsOptimizationGuide::InitializeOnIOThread,
                 base::Unretained(this), optimization_guide_service));
}

PreviewsOptimizationGuide::~PreviewsOptimizationGuide() {
  registrar_.reset();
}

void PreviewsOptimizationGuide::InitializeOnIOThread(
    optimization_guide::OptimizationGuideService* optimization_guide_service) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  registrar_.reset(new content::NotificationRegistrar);
  registrar_->Add(this, optimization_guide::NOTIFICATION_HINTS_PROCESSED,
                  content::Source<optimization_guide::OptimizationGuideService>(
                      optimization_guide_service));
}

bool PreviewsOptimizationGuide::IsWhitelisted(const net::URLRequest& request,
                                              PreviewsType type) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  return false;
}

void PreviewsOptimizationGuide::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  if (type == optimization_guide::NOTIFICATION_HINTS_PROCESSED) {
    const optimization_guide::Configuration* config =
        content::Details<const optimization_guide::Configuration>(details)
            .ptr();
    background_task_runner_->PostTask(
        FROM_HERE, base::Bind(&PreviewsOptimizationGuide::ProcessUnindexedHints,
                              base::Unretained(this), *config));
  }
}

void PreviewsOptimizationGuide::ProcessUnindexedHints(
    const optimization_guide::Configuration& config) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  Hints hints;
  for (const auto& hint : config.hints()) {
    hints.AddHint(hint.key());
  }
  io_task_runner_->PostTask(FROM_HERE,
                            base::Bind(&PreviewsOptimizationGuide::SwapHints,
                                       base::Unretained(this), hints));
}

void PreviewsOptimizationGuide::SwapHints(const Hints& hints) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  hints_.reset(&hints);
}

}  // namespace previews
