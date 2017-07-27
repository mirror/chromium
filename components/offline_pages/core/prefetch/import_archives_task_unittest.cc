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

  void ImportFile(const PrefetchItem& item,
                  const CompletedCallback& callback) override {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&TestPrefetchImporter::DoImportFile,
                              base::Unretained(this), item, callback));
  }

  const std::vector<PrefetchItem>& imported_items() const {
    return imported_items_;
  }

 private:
  void DoImportFile(const PrefetchItem& item,
                    const CompletedCallback& callback) {
    bool success = item.offline_id != kOfflineIDFailedToImport;
    callback.Run(success, item.offline_id);
    if (success)
      imported_items_.push_back(item);
  }

  std::vector<PrefetchItem> imported_items_;
};

bool AddItemSync(int64_t offline_id, sql::Connection* db) {
  static const char kSql[] =
      "INSERT INTO prefetch_items"
      " (offline_id, state, requested_url, final_archived_url,"
      " client_namespace, client_id, title, file_path, file_size,"
      " creation_time, freshness_time)"
      " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

  int64_t now_internal = base::Time::Now().ToInternalValue();
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, offline_id);
  statement.BindInt64(1, static_cast<int>(PrefetchItemState::DOWNLOADED));
  statement.BindString(2, kTestURL.spec());
  statement.BindString(3, kTestFinalURL.spec());
  statement.BindString(4, kTestClientID.name_space);
  statement.BindString(5, kTestClientID.id);
  statement.BindString16(6, kTestTitle);
  statement.BindString(7, kTestFilePath.AsUTF8Unsafe());
  statement.BindInt64(8, kTestFileSize);
  statement.BindInt64(9, now_internal);
  statement.BindInt64(10, now_internal);

  return statement.Run();
}

void AddItemDone(bool success) {}

PrefetchItem GetPrefetchItemSync(int64_t offline_id, sql::Connection* db) {
  static const char kSql[] =
      "SELECT * FROM prefetch_items WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, offline_id);

  PrefetchItem item;
  if (statement.Step())
    MakePrefetchItem(statement, &item);

  return item;
}

}  // namespace

class ImportArchivesTaskTest : public testing::Test {
 public:
  ImportArchivesTaskTest();
  ~ImportArchivesTaskTest() override = default;

  void SetUp() override;
  void TearDown() override;

  void PumpLoop();

  PrefetchItem GetPrefetchItem(int64_t offline_id);

  PrefetchStore* store() { return store_test_util_.store(); }
  PrefetchStoreTestUtil* store_util() { return &store_test_util_; }
  TestPrefetchImporter* importer() { return &test_importer_; }

 private:
  void GetPrefetchItemDone(PrefetchItem item);

  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  PrefetchStoreTestUtil store_test_util_;
  TestPrefetchImporter test_importer_;
  PrefetchItem item_;
};

ImportArchivesTaskTest::ImportArchivesTaskTest()
    : task_runner_(new base::TestSimpleTaskRunner),
      task_runner_handle_(task_runner_) {}

void ImportArchivesTaskTest::SetUp() {
  store_test_util_.BuildStoreInMemory();

  store()->Execute(base::BindOnce(&AddItemSync, kTestOfflineID),
                   base::BindOnce(&AddItemDone));
  PumpLoop();
  store()->Execute(base::BindOnce(&AddItemSync, kOfflineIDFailedToImport),
                   base::BindOnce(&AddItemDone));
  PumpLoop();
}

void ImportArchivesTaskTest::TearDown() {
  store_test_util_.DeleteStore();
  PumpLoop();
}

void ImportArchivesTaskTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

PrefetchItem ImportArchivesTaskTest::GetPrefetchItem(int64_t offline_id) {
  store()->Execute(base::BindOnce(&GetPrefetchItemSync, offline_id),
                   base::BindOnce(&ImportArchivesTaskTest::GetPrefetchItemDone,
                                  base::Unretained(this)));
  PumpLoop();
  return item_;
}

void ImportArchivesTaskTest::GetPrefetchItemDone(PrefetchItem item) {
  item_ = item;
}

TEST_F(ImportArchivesTaskTest, UpdateItemOnDownloadSuccess) {
  ImportArchivesTask task(store(), importer());
  task.Run();
  PumpLoop();

  PrefetchItem item(GetPrefetchItem(kTestOfflineID));
  EXPECT_EQ(PrefetchItemState::FINISHED, item.state);
  EXPECT_EQ(PrefetchItemErrorCode::SUCCESS, item.error_code);

  PrefetchItem item2(GetPrefetchItem(kOfflineIDFailedToImport));
  EXPECT_EQ(PrefetchItemState::FINISHED, item2.state);
  EXPECT_EQ(PrefetchItemErrorCode::IMPORT_ERROR, item2.error_code);

  ASSERT_EQ(1u, importer()->imported_items().size());
  EXPECT_EQ(kTestOfflineID, importer()->imported_items()[0].offline_id);
}

}  // namespace offline_pages
