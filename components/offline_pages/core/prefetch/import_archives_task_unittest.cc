// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/import_archives_task.h"

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
const int64_t kOfflineIDFailedToImport = 2222;
const GURL kTestURL("http://sample.org");
const GURL kTestFinalURL("https://sample.org");
const ClientId kTestClientID("Foo", "Bar");
const base::string16 kTestTitle = base::ASCIIToUTF16("Hello");
const base::FilePath kTestFilePath(FILE_PATH_LITERAL("foo"));
const int64_t kTestFileSize = 88888;

class TestPrefetchImporter : public PrefetchImporter {
 public:
  TestPrefetchImporter() {}
  ~TestPrefetchImporter() override = default;

  void ImportArchive(const ArchiveInfo& info,
                     const CompletedCallback& callback) override {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&TestPrefetchImporter::DoImportArchive,
                              base::Unretained(this), info, callback));
  }

  const std::vector<int64_t>& imported_offline_ids() const {
    return imported_offline_ids_;
  }

 private:
  void DoImportArchive(const ArchiveInfo& info,
                       const CompletedCallback& callback) {
    bool success = info.offline_id != kOfflineIDFailedToImport;
    callback.Run(success, info.offline_id);
    if (success)
      imported_offline_ids_.push_back(info.offline_id);
  }

  std::vector<int64_t> imported_offline_ids_;
};

}  // namespace

class ImportArchivesTaskTest : public testing::Test {
 public:
  ImportArchivesTaskTest();
  ~ImportArchivesTaskTest() override = default;

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

ImportArchivesTaskTest::ImportArchivesTaskTest()
    : task_runner_(new base::TestSimpleTaskRunner),
      task_runner_handle_(task_runner_),
      store_test_util_(task_runner_) {}

void ImportArchivesTaskTest::SetUp() {
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
  int64_t result = store_test_util_.InsertPrefetchItem(item);
  EXPECT_EQ(kTestOfflineID, result);

  item.offline_id = kOfflineIDFailedToImport;
  result = store_test_util_.InsertPrefetchItem(item);
  EXPECT_EQ(kOfflineIDFailedToImport, result);
}

void ImportArchivesTaskTest::TearDown() {
  store_test_util_.DeleteStore();
  PumpLoop();
}

void ImportArchivesTaskTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

TEST_F(ImportArchivesTaskTest, UpdateItemOnDownloadSuccess) {
  ImportArchivesTask task(store(), importer());
  task.Run();
  PumpLoop();

  std::unique_ptr<PrefetchItem> item =
      store_util()->GetPrefetchItem(kTestOfflineID);
  EXPECT_EQ(PrefetchItemState::FINISHED, item->state);
  EXPECT_EQ(PrefetchItemErrorCode::SUCCESS, item->error_code);

  std::unique_ptr<PrefetchItem> item2 =
      store_util()->GetPrefetchItem(kOfflineIDFailedToImport);
  EXPECT_EQ(PrefetchItemState::FINISHED, item2->state);
  EXPECT_EQ(PrefetchItemErrorCode::IMPORT_ERROR, item2->error_code);

  ASSERT_EQ(1u, importer()->imported_offline_ids().size());
  EXPECT_EQ(kTestOfflineID, importer()->imported_offline_ids().back());
}

}  // namespace offline_pages
