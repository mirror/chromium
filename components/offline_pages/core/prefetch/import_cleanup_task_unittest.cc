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

  void ImportArchive(const PrefetchArchiveInfo& archive) override {
    outstanding_import_offline_ids_.emplace(archive.offline_id);
  }

  void MarkImportCompleted(int64_t offline_id) override {
    outstanding_import_offline_ids_.erase(offline_id);
  }

  std::set<int64_t> GetOutstandingImports() const override {
    return outstanding_import_offline_ids_;
  }

 private:
  std::set<int64_t> outstanding_import_offline_ids_;
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
  EXPECT_EQ(PrefetchItemErrorCode::IMPORT_LOST, store_item->error_code);

  std::unique_ptr<PrefetchItem> store_item2 =
      store_util()->GetPrefetchItem(item2.offline_id);
  ASSERT_TRUE(store_item2);
  EXPECT_EQ(item2, *store_item2);

  std::unique_ptr<PrefetchItem> store_item3 =
      store_util()->GetPrefetchItem(item3.offline_id);
  ASSERT_TRUE(store_item3);
  EXPECT_EQ(PrefetchItemState::FINISHED, store_item3->state);
  EXPECT_EQ(PrefetchItemErrorCode::IMPORT_LOST, store_item3->error_code);
}

TEST_F(ImportCleanupTaskTest, NoCleanupForOutstandingImport) {
  PrefetchItem item =
      item_generator()->CreateItem(PrefetchItemState::IMPORTING);
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item));

  PrefetchArchiveInfo archive_info;
  archive_info.offline_id = item.offline_id;
  importer()->ImportArchive(archive_info);

  ImportCleanupTask task(store(), importer());
  task.Run();
  RunUntilIdle();

  // The item is intact since it is in the outstanding list.
  std::unique_ptr<PrefetchItem> store_item =
      store_util()->GetPrefetchItem(item.offline_id);
  ASSERT_TRUE(store_item);
  EXPECT_EQ(item, *store_item);

  // Mark it as completed in order to remove it from the outstanding list.
  importer()->MarkImportCompleted(item.offline_id);

  ImportCleanupTask task2(store(), importer());
  task2.Run();
  RunUntilIdle();

  // The item should now be cleaned up.
  store_item = store_util()->GetPrefetchItem(item.offline_id);
  ASSERT_TRUE(store_item);
  EXPECT_EQ(PrefetchItemState::FINISHED, store_item->state);
  EXPECT_EQ(PrefetchItemErrorCode::IMPORT_LOST, store_item->error_code);
}

}  // namespace offline_pages
