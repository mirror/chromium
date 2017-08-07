// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/import_completed_task.h"

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

}  // namespace

class ImportCompletedTaskTest : public testing::Test {
 public:
  ImportCompletedTaskTest();
  ~ImportCompletedTaskTest() override = default;

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

ImportCompletedTaskTest::ImportCompletedTaskTest()
    : task_runner_(new base::TestSimpleTaskRunner),
      task_runner_handle_(task_runner_),
      store_test_util_(task_runner_) {}

void ImportCompletedTaskTest::SetUp() {
  store_test_util_.BuildStoreInMemory();

  PrefetchItem item;
  item.offline_id = kTestOfflineID;
  item.state = PrefetchItemState::IMPORTING;
  item.creation_time = base::Time::Now();
  item.freshness_time = item.creation_time;
  EXPECT_TRUE(store_test_util_.InsertPrefetchItem(item));
}

void ImportCompletedTaskTest::TearDown() {
  store_test_util_.DeleteStore();
  PumpLoop();
}

void ImportCompletedTaskTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

TEST_F(ImportCompletedTaskTest, ImportSucceeded) {
  ImportCompletedTask task(store(), kTestOfflineID, true);
  task.Run();
  PumpLoop();

  std::unique_ptr<PrefetchItem> item =
      store_util()->GetPrefetchItem(kTestOfflineID);
  EXPECT_EQ(PrefetchItemState::FINISHED, item->state);
  EXPECT_EQ(PrefetchItemErrorCode::SUCCESS, item->error_code);
}

TEST_F(ImportCompletedTaskTest, ImportFailed) {
  ImportCompletedTask task(store(), kTestOfflineID, false);
  task.Run();
  PumpLoop();

  std::unique_ptr<PrefetchItem> item =
      store_util()->GetPrefetchItem(kTestOfflineID);
  EXPECT_EQ(PrefetchItemState::FINISHED, item->state);
  EXPECT_EQ(PrefetchItemErrorCode::IMPORT_ERROR, item->error_code);
}

}  // namespace offline_pages
