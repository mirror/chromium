// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/offline_page_metadata_store_test_util.h"

#include "base/bind.h"
#include "base/callback_forward.h"
#include "components/offline_pages/core/model/get_pages_task.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

OfflinePageMetadataStoreTestUtil::OfflinePageMetadataStoreTestUtil(
    scoped_refptr<base::TestSimpleTaskRunner> task_runner)
    : task_runner_(task_runner) {}

OfflinePageMetadataStoreTestUtil::~OfflinePageMetadataStoreTestUtil() {}

void OfflinePageMetadataStoreTestUtil::BuildStore() {
  if (!temp_directory_.CreateUniqueTempDir()) {
    DVLOG(1) << "temp_directory_ not created";
    return;
  }

  store_.reset(
      new OfflinePageMetadataStoreSQL(task_runner_, temp_directory_.GetPath()));
}

void OfflinePageMetadataStoreTestUtil::BuildStoreInMemory() {
  store_.reset(new OfflinePageMetadataStoreSQL(task_runner_));
}

void OfflinePageMetadataStoreTestUtil::DeleteStore() {
  store_.reset();
  task_runner_->RunUntilIdle();
}

std::unique_ptr<OfflinePageMetadataStoreSQL>
OfflinePageMetadataStoreTestUtil::ReleaseStore() {
  return std::move(store_);
}

void OfflinePageMetadataStoreTestUtil::InsertItem(const OfflinePageItem& page) {
  ItemActionStatus status;
  store_->AddOfflinePage(
      page, base::Bind([](ItemActionStatus* out_status,
                          ItemActionStatus status) { *out_status = status; },
                       &status));
  task_runner_->RunUntilIdle();
  EXPECT_EQ(ItemActionStatus::SUCCESS, status);
}

OfflinePageItem OfflinePageMetadataStoreTestUtil::GetPageByOfflineId(
    int64_t offline_id) {
  OfflinePageItem out_page;
  auto task = GetPagesTask::CreateTaskMatchingOfflineId(
      store(),
      base::Bind(
          [](OfflinePageItem* out_page, const OfflinePageItem* page) {
            if (page)
              *out_page = *page;
          },
          &out_page),
      offline_id);
  task->Run();
  task_runner_->RunUntilIdle();
  return out_page;
}

}  // namespace offline_pages
