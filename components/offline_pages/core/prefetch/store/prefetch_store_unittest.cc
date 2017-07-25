// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/store/prefetch_store.h"

#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/client_id.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_test_util.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace offline_pages {
namespace {

int CountPrefetchItems(sql::Connection* db) {
  // Not starting transaction as this is a single read.
  static const char kSql[] = "SELECT COUNT(offline_id) FROM prefetch_items";
  sql::Statement statement(db->GetUniqueStatement(kSql));
  if (statement.Step())
    return statement.ColumnInt(0);

  return -1;
}

}  // namespace

class PrefetchStoreTest : public testing::Test {
 public:
  PrefetchStoreTest();
  ~PrefetchStoreTest() override = default;

  void SetUp() override { store_test_util_.BuildStoreInMemory(); }

  void TearDown() override {
    store_test_util_.DeleteStore();
    PumpLoop();
  }

  PrefetchStore* store() { return store_test_util_.store(); }

  PrefetchStoreTestUtil* store_util() { return &store_test_util_; }

  void PumpLoop() { task_runner_->RunUntilIdle(); }

 private:
  PrefetchStoreTestUtil store_test_util_;
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
};

PrefetchStoreTest::PrefetchStoreTest()
    : task_runner_(new base::TestSimpleTaskRunner),
      task_runner_handle_(task_runner_) {}

TEST_F(PrefetchStoreTest, InitializeStore) {
  store()->Execute<int>(base::BindOnce(&CountPrefetchItems),
                        store_util()->ExpectEqCallback<int>(0));
  PumpLoop();
}

TEST_F(PrefetchStoreTest, WriteAndLoadOneItem) {
  PrefetchItem item1;
  item1.offline_id = 77L;
  item1.guid = "A";
  item1.client_id = ClientId("B", "C");
  item1.state = PrefetchItemState::AWAITING_GCM;
  item1.url = GURL("http://test.com");
  item1.final_archived_url = GURL("http://test.com/final");
  item1.generate_bundle_attempts = 10;
  item1.get_operation_attempts = 11;
  item1.download_operation_attempts = 12;
  item1.operation_name = "D";
  item1.archive_body_name = "E";
  item1.archive_body_length = 20;
  item1.creation_time = base::Time::FromJavaTime(1000L);
  item1.freshness_time = base::Time::FromJavaTime(2000L);
  item1.error_code = PrefetchItemErrorCode::EXPIRED;

  store_util()->InsertItem(item1, store_util()->ExpectTrueCallback());
  PumpLoop();

  std::set<PrefetchItem> all_items;
  store_util()->GetAllItems(&all_items,
                            store_util()->ExpectEqCallback<std::size_t>(1U));
  PumpLoop();

  EXPECT_EQ(1U, all_items.count(item1));
}

}  // namespace offline_pages
