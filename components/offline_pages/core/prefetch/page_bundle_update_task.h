// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PAGE_BUNDLE_UPDATE_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PAGE_BUNDLE_UPDATE_TASK_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {
class PrefetchDispatcher;
class PrefetchStore;

// Task that writes down the result of a prefetch operation and optionally
// schedules action tasks.
class PageBundleUpdateTask : public Task {
 public:
  // The result is whether we need more action tasks to run right now.
  using PageBundleUpdateResult = bool;

  PageBundleUpdateTask(PrefetchStore* store,
                       PrefetchDispatcher* dispatcher,
                       const std::string& operation_name,
                       const std::vector<RenderPageInfo>& pages);
  ~PageBundleUpdateTask() override;

  // Task implementation.
  void Run() override;

 private:
  void FinishedWork(PageBundleUpdateResult result);

  // Owned by PrefetchService which also owns |this|.
  PrefetchStore* store_;
  // Owned by PrefetchService which also owns |this|.
  PrefetchDispatcher* dispatcher_;
  std::string operation_name_;
  std::vector<RenderPageInfo> pages_;

  base::WeakPtrFactory<PageBundleUpdateTask> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PageBundleUpdateTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PAGE_BUNDLE_UPDATE_TASK_H_
