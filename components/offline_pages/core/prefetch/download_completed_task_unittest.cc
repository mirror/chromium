// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/download_completed_task.h"

#include <string>
#include <vector>

#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
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
const char kTestGUID[] = "1a150628-1b56-44da-a85a-c575120af180";
const base::FilePath kTestFilePath(FILE_PATH_LITERAL("foo"));
const int64_t kTestFileSize = 88888;

bool AddItemSync(sql::Connection* db) {
  static const char kSql[] =
      "INSERT INTO prefetch_items"
      " (offline_id, guid, creation_time, freshness_time)"
      " VALUES (?, ?, ?, ?)";

  int64_t now_internal = base::Time::Now().ToInternalValue();
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, kTestOfflineID);
  statement.BindString(1, kTestGUID);
  statement.BindInt64(2, now_internal);
  statement.BindInt64(3, now_internal);

  return statement.Run();
}

void AddItemDone(bool success) {}

PrefetchItem GetPrefetchItemByGUIDSync(const std::string& guid,
                                       sql::Connection* db) {
  static const char kSql[] = "SELECT * FROM prefetch_items WHERE guid = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindString(0, guid);

  PrefetchItem item;
  if (statement.Step())
    MakePrefetchItem(statement, &item);

  return item;
}

}  // namespace

class DownloadCompletedTaskTest : public testing::Test {
 public:
  DownloadCompletedTaskTest();
  ~DownloadCompletedTaskTest() override = default;

  void SetUp() override;
  void TearDown() override;

  void PumpLoop();

  PrefetchItem GetPrefetchItemByGUID(const std::string& guid);

  PrefetchStore* store() { return store_test_util_.store(); }
  PrefetchStoreTestUtil* store_util() { return &store_test_util_; }

 private:
  void GetPrefetchItemDone(PrefetchItem item);

  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  PrefetchStoreTestUtil store_test_util_;
  PrefetchItem item_;
};

DownloadCompletedTaskTest::DownloadCompletedTaskTest()
    : task_runner_(new base::TestSimpleTaskRunner),
      task_runner_handle_(task_runner_) {}

void DownloadCompletedTaskTest::SetUp() {
  store_test_util_.BuildStoreInMemory();

  store()->Execute(base::BindOnce(&AddItemSync), base::BindOnce(&AddItemDone));
  PumpLoop();
}

void DownloadCompletedTaskTest::TearDown() {
  store_test_util_.DeleteStore();
  PumpLoop();
}

void DownloadCompletedTaskTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

PrefetchItem DownloadCompletedTaskTest::GetPrefetchItemByGUID(
    const std::string& guid) {
  store()->Execute(
      base::BindOnce(&GetPrefetchItemByGUIDSync, guid),
      base::BindOnce(&DownloadCompletedTaskTest::GetPrefetchItemDone,
                     base::Unretained(this)));
  PumpLoop();
  return item_;
}

void DownloadCompletedTaskTest::GetPrefetchItemDone(PrefetchItem item) {
  item_ = item;
}

TEST_F(DownloadCompletedTaskTest, UpdateItemOnDownloadSuccess) {
  PrefetchDownloadResult download_result(kTestGUID, kTestFilePath,
                                         kTestFileSize);
  DownloadCompletedTask task(store(), download_result);
  task.Run();
  PumpLoop();

  PrefetchItem item(GetPrefetchItemByGUID(kTestGUID));
  EXPECT_EQ(PrefetchItemState::DOWNLOADED, item.state);
  EXPECT_EQ(kTestGUID, item.guid);
  EXPECT_EQ(kTestFilePath, item.file_path);
  EXPECT_EQ(kTestFileSize, item.file_size);
}

TEST_F(DownloadCompletedTaskTest, UpdateItemOnDownloadError) {
  PrefetchDownloadResult download_result;
  download_result.download_id = kTestGUID;
  download_result.success = false;
  DownloadCompletedTask task(store(), download_result);
  task.Run();
  PumpLoop();

  PrefetchItem item(GetPrefetchItemByGUID(kTestGUID));
  EXPECT_EQ(PrefetchItemState::FINISHED, item.state);
  EXPECT_EQ(PrefetchItemErrorCode::DOWNLOAD_ERROR, item.error_code);
  EXPECT_EQ(kTestGUID, item.guid);
  EXPECT_TRUE(item.file_path.empty());
  EXPECT_EQ(0, item.file_size);
}

}  // namespace offline_pages
