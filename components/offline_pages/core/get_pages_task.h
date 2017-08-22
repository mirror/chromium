// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_GET_PAGES_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_GET_PAGES_TASK_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {

struct OfflinePageItem;
class OfflinePageModelQuery;

class GetPagesTask : public Task {
 public:
  GetPagesTask(OfflinePageMetadataStore* store,
               std::unique_ptr<OfflinePageModelQuery> query,
               const MultipleOfflinePageItemCallback& callback);
  ~GetPagesTask() override;

  // Task implementation.
  void Run() override;

 private:
  void OnGetAllPagesDone(const std::vector<OfflinePageItem>& pages);

  // Not owned.
  OfflinePageMetadataStore* store_;

  std::unique_ptr<OfflinePageModelQuery> query_;
  MultipleOfflinePageItemCallback callback_;
  std::vector<OfflinePageItem> pages_;

  base::WeakPtrFactory<GetPagesTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(GetPagesTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_GET_PAGES_TASK_H_
