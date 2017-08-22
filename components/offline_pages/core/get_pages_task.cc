// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/get_pages_task.h"

#include "base/bind.h"
#include "components/offline_pages/core/offline_page_metadata_store.h"
#include "components/offline_pages/core/offline_page_types.h"

namespace offline_pages {

GetPagesTask::GetPagesTask(OfflinePageMetadataStore* store,
                           std::unique_ptr<OfflinePageModelQuery> query,
                           const MultipleOfflinePageItemCallback& callback)
    : store_(store),
      query_(std::move(query)),
      callback_(callback),
      weak_ptr_factory_(this) {}

GetPagesTask::~GetPagesTask() {}

void GetPagesTask::Run() {
  DCHECK(store_);
  store_->GetOfflinePages(base::Bind(&GetPagesTask::OnGetAllPagesDone,
                                     weak_ptr_factory_.GetWeakPtr()));
}

void GetPagesTask::OnGetAllPagesDone(
    const MultipleOfflinePageItemResult& pages) {
  for (const auto& page : pages) {
    DLOG(ERROR) << "ROMAX OnGetAllPagesDone: " << page.offline_id;
    if (query_->Matches(page))
      pages_.emplace_back(page);
  }
  callback_.Run(pages_);
  TaskComplete();
}

}  // namespace offline_pages
