// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/import_archive_task.h"

#include <string>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "components/offline_pages/core/prefetch/prefetch_importer.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_test_util.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

namespace {

const int64_t kTestOfflineID = 1111;
const GURL kTestURL("http://sample.org");
const GURL kTestFinalURL("https://sample.org");
const ClientId kTestClientID("Foo", "Bar");
const base::string16 kTestTitle = base::ASCIIToUTF16("Hello");
const base::FilePath kTestFilePath(FILE_PATH_LITERAL("foo"));
const int64_t kTestFileSize = 88888;

class TestPrefetchImporter : public PrefetchImporter {
 public:
  TestPrefetchImporter() : PrefetchImporter(nullptr) {}
  ~TestPrefetchImporter() override = default;

  void ImportArchive(const PrefetchArchiveInfo& archive) override {
    archive_ = archive;
  }

  const PrefetchArchiveInfo& archive() const { return archive_; }

 private:
  PrefetchArchiveInfo archive_;
};

}  // namespace

class ImportArchiveTaskTest : public testing::Test {
 public:
  ImportArchiveTaskTest();
  ~ImportArchiveTaskTest() override = default;

  void SetUp() override;
  void TearDown() override;

  void PumpLoop();

  PrefetchStore* store() { return store_test_util_.store(); }
  PrefetchStoreTestUtil* store_util() { return &store_test_util_; }
  TestPrefetchImporter* importer() { return &test_importer_; }

 private:
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  PrefetchStoreTestUtil store_test_util_;
  TestPrefetchImporter test_importer_;
};

ImportArchiveTaskTest::ImportArchiveTaskTest()
    : task_runner_(new base::TestSimpleTaskRunner),
      task_runner_handle_(task_runner_),
      store_test_util_(task_runner_) {}

void ImportArchiveTaskTest::SetUp() {
  store_test_util_.BuildStoreInMemory();

  PrefetchItem item;
  item.offline_id = kTestOfflineID;
  item.state = PrefetchItemState::DOWNLOADED;
  item.url = kTestURL;
  item.final_archived_url = kTestFinalURL;
  item.client_id = kTestClientID;
  item.title = kTestTitle;
  item.file_path = kTestFilePath;
  item.file_size = kTestFileSize;
  item.creation_time = base::Time::Now();
  item.freshness_time = item.creation_time;
  EXPECT_TRUE(store_test_util_.InsertPrefetchItem(item));
}

void ImportArchiveTaskTest::TearDown() {
  store_test_util_.DeleteStore();
  PumpLoop();
}

void ImportArchiveTaskTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

TEST_F(ImportArchiveTaskTest, Importing) {
  ImportArchiveTask task(store(), importer());
  task.Run();
  PumpLoop();

  std::unique_ptr<PrefetchItem> item =
      store_util()->GetPrefetchItem(kTestOfflineID);
  EXPECT_EQ(PrefetchItemState::IMPORTING, item->state);

  EXPECT_EQ(kTestOfflineID, importer()->archive().offline_id);
  EXPECT_EQ(kTestFilePath, importer()->archive().file_path);
  EXPECT_EQ(kTestFileSize, importer()->archive().file_size);
}

}  // namespace offline_pages
