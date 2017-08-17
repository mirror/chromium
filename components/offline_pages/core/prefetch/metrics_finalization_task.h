// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_METRICS_FINALIZATION_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_METRICS_FINALIZATION_TASK_H_

#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {
class PrefetchStore;

// Task that attempts to start archiving the URLs the prefetch service has
// determined are viable to prefetch.
class MetricsFinalizationTask : public Task {
 public:
  MetricsFinalizationTask(PrefetchStore* prefetch_store);
  ~MetricsFinalizationTask() override;

  // Task implementation.
  void Run() override;

 private:
  void MetricsFinalized(bool result);

  PrefetchStore* prefetch_store_;

  base::WeakPtrFactory<MetricsFinalizationTask> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(MetricsFinalizationTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_METRICS_FINALIZATION_TASK_H_
