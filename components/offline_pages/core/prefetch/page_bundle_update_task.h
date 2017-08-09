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

// Task that writes down the result of a prefetch networking operation and
// optionally schedules action tasks.  Results appear in both GetOperation
// and GeneratePageBundle responses.
//
// This adds a single task that can be used for either event.  It does 3
// things:
// - Any pages in the result that have succeeded are updated to
//   RECEIVED_BUNDLE and body name / length are updated.
// - Any pages in the result that failed are updated to FINISHED
//   with the ARCHIVING_FAILED error code.
// - Any pages in the result that are "Pending" and match rows with an
//   empty operation name will update to AWAITING_GCM and save the
//   operation name.
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
