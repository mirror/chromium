// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/sent_get_operation_cleanup_task.h"

#include <string>

#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_test_util.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

namespace {
const int64_t kTestOfflineID = 1111;
const int64_t kTestOfflineID2 = 223344;
const int64_t kTestOfflineID3 = 987;
}  // namespace

class SentGetOperationCleanupTaskTest : public testing::Test {
 public:
  SentGetOperationCleanupTaskTest();
  ~SentGetOperationCleanupTaskTest() override = default;

  void SetUp() override;
  void TearDown() override;

  void PumpLoop();

  PrefetchStore* store() { return store_test_util_.store(); }
  PrefetchStoreTestUtil* store_util() { return &store_test_util_; }

 private:
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  PrefetchStoreTestUtil store_test_util_;
};

SentGetOperationCleanupTaskTest::SentGetOperationCleanupTaskTest()
    : task_runner_(new base::TestSimpleTaskRunner),
      task_runner_handle_(task_runner_),
      store_test_util_(task_runner_) {}

void SentGetOperationCleanupTaskTest::SetUp() {
  store_test_util_.BuildStoreInMemory();

  PrefetchItem item;
  item.offline_id = kTestOfflineID;
  item.state = PrefetchItemState::SENT_GET_OPERATION;
  item.get_operation_attempts = 1;
  item.creation_time = base::Time::Now();
  item.freshness_time = item.creation_time;
  EXPECT_TRUE(store_test_util_.InsertPrefetchItem(item));

  PrefetchItem item2;
  item2.offline_id = kTestOfflineID2;
  item2.state = PrefetchItemState::SENT_GET_OPERATION;
  item2.get_operation_attempts =
      SentGetOperationCleanupTask::kMaxGetOperationAttempts + 1;
  item2.creation_time = base::Time::Now();
  item2.freshness_time = item.creation_time;
  EXPECT_TRUE(store_test_util_.InsertPrefetchItem(item2));

  PrefetchItem item3;
  item3.offline_id = kTestOfflineID3;
  item3.state = PrefetchItemState::NEW_REQUEST;
  item3.creation_time = base::Time::Now();
  item3.freshness_time = item.creation_time;
  EXPECT_TRUE(store_test_util_.InsertPrefetchItem(item3));
}

void SentGetOperationCleanupTaskTest::TearDown() {
  store_test_util_.DeleteStore();
  PumpLoop();
}

void SentGetOperationCleanupTaskTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

TEST_F(SentGetOperationCleanupTaskTest, Cleanup) {
  SentGetOperationCleanupTask task(store());
  task.Run();
  PumpLoop();

  // Two items are updated.
  std::unique_ptr<PrefetchItem> item =
      store_util()->GetPrefetchItem(kTestOfflineID);
  EXPECT_EQ(PrefetchItemState::RECEIVED_GCM, item->state);
  EXPECT_EQ(1, item->get_operation_attempts);

  item = store_util()->GetPrefetchItem(kTestOfflineID2);
  EXPECT_EQ(PrefetchItemState::FINISHED, item->state);
  EXPECT_EQ(PrefetchItemErrorCode::GET_OPERATION_MAX_ATTEMPTS_REACHED,
            item->error_code);

  // One item is not updated.
  item = store_util()->GetPrefetchItem(kTestOfflineID3);
  EXPECT_EQ(PrefetchItemState::NEW_REQUEST, item->state);
}

}  // namespace offline_pages
