// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_TASKS_ADD_PAGE_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_TASKS_ADD_PAGE_TASK_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "components/offline_pages/core/offline_store_types.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {

class OfflinePageMetadataStoreSQL;

class AddPageTask : public Task {
 public:
  typedef base::Callback<void(AddPageResult, const OfflinePageItem&)>
      AddPageTaskCallback;

  AddPageTask(OfflinePageMetadataStoreSQL* store,
              const OfflinePageItem& offline_page,
              const AddPageTaskCallback& callback);
  ~AddPageTask() override;

  // Task implementation.
  void Run() override;

 private:
  void OnAddPageDone(ItemActionStatus status);
  void InformAddPageDone(AddPageResult result);

  OfflinePageMetadataStoreSQL* store_;
  OfflinePageItem offline_page_;
  AddPageTaskCallback callback_;

  base::WeakPtrFactory<AddPageTask> weak_ptr_factory;
  DISALLOW_COPY_AND_ASSIGN(AddPageTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_TASKS_ADD_PAGE_TASK_H
