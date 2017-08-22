// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_UPDATE_PAGE_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_UPDATE_PAGE_TASK_H_

#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "components/offline_pages/core/offline_page_types.h"

namespace offline_pages {

class UpdatePageTask : public Task {
 public:
  UpdatePageTask(OfflinePageModel* model, int64_t offline_id);
  ~UpdatePageTask();

  // Task implementation.
  void Run() override;

 private:
  void MarkPageAccessed();
  void OnMarkPageAccessedDone(std::unique_ptr<OfflinePageUpdateResult> result);

  int64_t offline_id;

  base::WeakPtrFactory<UpdatePageTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(UpdatePageTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_UPDATE_PAGE_TASK_H_
