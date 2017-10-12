// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/import_cleanup_task.h"

#include "components/offline_pages/core/prefetch/prefetch_importer.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "components/offline_pages/core/prefetch/task_test_base.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

namespace {

class TestPrefetchImporter : public PrefetchImporter {
 public:
  TestPrefetchImporter() : PrefetchImporter(nullptr) {}
  ~TestPrefetchImporter() override = default;

  void ImportArchive(const PrefetchArchiveInfo& archive) override {}
  std::list<int64_t> GetOngoingImports() const override {
    return importing_offline_ids_;
  }

  void AddItemToImport(int64_t offline_id) {
    importing_offline_ids_.push_back(offline_id);
  }

 private:
  std::list<int64_t> importing_offline_ids_;
};

}  // namespace

class ImportCleanupTaskTest : public TaskTestBase {
 public:
  ImportCleanupTaskTest() = default;
  ~ImportCleanupTaskTest() override = default;

  TestPrefetchImporter* importer() { return &test_importer_; }

 private:
  TestPrefetchImporter test_importer_;
};

TEST_F(ImportCleanupTaskTest, StoreFailure) {
  store_util()->SimulateInitializationError();

  ImportCleanupTask task(store(), importer());
  task.Run();
  RunUntilIdle();
}

TEST_F(ImportCleanupTaskTest, DoCleanup) {
  PrefetchItem item =
      item_generator()->CreateItem(PrefetchItemState::IMPORTING);
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item));

  PrefetchItem item2 =
      item_generator()->CreateItem(PrefetchItemState::DOWNLOADING);
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item2));

  PrefetchItem item3 =
      item_generator()->CreateItem(PrefetchItemState::IMPORTING);
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item3));

  ImportCleanupTask task(store(), importer());
  task.Run();
  RunUntilIdle();

  std::unique_ptr<PrefetchItem> store_item =
      store_util()->GetPrefetchItem(item.offline_id);
  ASSERT_TRUE(store_item);
  EXPECT_EQ(PrefetchItemState::FINISHED, store_item->state);
  EXPECT_EQ(PrefetchItemErrorCode::IMPORT_ABORTED, store_item->error_code);

  std::unique_ptr<PrefetchItem> store_item2 =
      store_util()->GetPrefetchItem(item2.offline_id);
  ASSERT_TRUE(store_item2);
  EXPECT_EQ(item2, *store_item2);

  std::unique_ptr<PrefetchItem> store_item3 =
      store_util()->GetPrefetchItem(item3.offline_id);
  ASSERT_TRUE(store_item3);
  EXPECT_EQ(PrefetchItemState::FINISHED, store_item3->state);
  EXPECT_EQ(PrefetchItemErrorCode::IMPORT_ABORTED, store_item3->error_code);
}

TEST_F(ImportCleanupTaskTest, NoCleanupForOngoingImport) {
  PrefetchItem item =
      item_generator()->CreateItem(PrefetchItemState::IMPORTING);
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item));

  importer()->AddItemToImport(item.offline_id);

  ImportCleanupTask task(store(), importer());
  task.Run();
  RunUntilIdle();

  std::unique_ptr<PrefetchItem> store_item =
      store_util()->GetPrefetchItem(item.offline_id);
  EXPECT_EQ(item, *store_item);
}

}  // namespace offline_pages
