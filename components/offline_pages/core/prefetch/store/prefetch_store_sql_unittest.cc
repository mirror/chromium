// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/store/prefetch_store_sql.h"

#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_sql_test_util.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {
namespace {

int CountPrefetchItems(sql::Connection* db) {
  // Not starting transaction as this is a single read.
  const char kSql[] = "SELECT COUNT(offline_id) FROM prefetch_items";
  sql::Statement statement(db->GetUniqueStatement(kSql));
  if (statement.Step())
    return statement.ColumnInt(0);

  return -1;
}

void CountPrefetchItemsResult(int expected_count, int actual_count) {
  EXPECT_EQ(expected_count, actual_count);
}

}  // namespace
class PrefetchStoreSqlTest : public testing::Test {
 public:
  PrefetchStoreSqlTest();
  ~PrefetchStoreSqlTest() override = default;

  void SetUp() override { store_test_util_.BuildStore(); }

  PrefetchStoreSQL* store() { return store_test_util_.store(); }

  void PumpLoop() { task_runner_->RunUntilIdle(); }

 private:
  PrefetchStoreSQLTestUtil store_test_util_;
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
};

PrefetchStoreSqlTest::PrefetchStoreSqlTest()
    : task_runner_(new base::TestSimpleTaskRunner),
      task_runner_handle_(task_runner_) {}

TEST_F(PrefetchStoreSqlTest, InitializeStore) {
  store()->Execute<int>(base::BindOnce(&CountPrefetchItems),
                        base::BindOnce(&CountPrefetchItemsResult, 0));
  PumpLoop();
}
}  // namespace offline_pages
