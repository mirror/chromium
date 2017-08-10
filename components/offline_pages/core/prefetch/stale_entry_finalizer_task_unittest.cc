// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/stale_entry_finalizer_task.h"

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/prefetch/mock_prefetch_item_generator.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_test_util.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_utils.h"
#include "components/offline_pages/core/prefetch/task_test_base.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

auto& kBucket1FreshPeriod = StaleEntryFinalizerTask::kBucket1FreshPeriod;
auto& kBucket2FreshPeriod = StaleEntryFinalizerTask::kBucket2FreshPeriod;
auto& kBucket3FreshPeriod = StaleEntryFinalizerTask::kBucket3FreshPeriod;
auto& kBucket1StatesToExpire = StaleEntryFinalizerTask::kBucket1StatesToExpire;
auto& kBucket2StatesToExpire = StaleEntryFinalizerTask::kBucket2StatesToExpire;
auto& kBucket3StatesToExpire = StaleEntryFinalizerTask::kBucket3StatesToExpire;

class StaleEntryFinalizerTaskTest : public TaskTestBase {
 public:
  StaleEntryFinalizerTaskTest() = default;
  ~StaleEntryFinalizerTaskTest() override = default;

  void SetUp() override;
  void TearDown() override;

 protected:
  std::unique_ptr<StaleEntryFinalizerTask> stale_finalizer_task_;
};

void StaleEntryFinalizerTaskTest::SetUp() {
  TaskTestBase::SetUp();
  stale_finalizer_task_ = base::MakeUnique<StaleEntryFinalizerTask>(store());
}

void StaleEntryFinalizerTaskTest::TearDown() {
  stale_finalizer_task_.reset();
  TaskTestBase::TearDown();
}

TEST_F(StaleEntryFinalizerTaskTest, EmptyRun) {
  std::set<PrefetchItem> no_items;
  EXPECT_EQ(0U, store_util()->GetAllItems(&no_items));

  // Execute the expiration task.
  ExpectTaskCompletes(stale_finalizer_task_.get());
  stale_finalizer_task_->Run();
  RunUntilIdle();
  EXPECT_TRUE(stale_finalizer_task_->ran_successfully());
  EXPECT_EQ(0U, store_util()->GetAllItems(&no_items));
}

TEST_F(StaleEntryFinalizerTaskTest, ExpireStaleItemsOnly) {
  const base::Time fake_now = base::Time() + base::TimeDelta::FromDays(100);
  stale_finalizer_task_->SetNowCallbackForTesting(base::BindRepeating(
      [](base::Time t) -> base::Time { return t; }, fake_now));

  // Insert fresh and stale items for each bucket.
  PrefetchItem item1_b1_fresh(
      item_generator()->CreateItem(kBucket1StatesToExpire[0]));
  item1_b1_fresh.freshness_time =
      fake_now - kBucket1FreshPeriod + base::TimeDelta::FromMinutes(1);
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item1_b1_fresh));
  PrefetchItem item2_b1_stale(
      item_generator()->CreateItem(kBucket1StatesToExpire[0]));
  item2_b1_stale.freshness_time =
      fake_now - kBucket1FreshPeriod - base::TimeDelta::FromMinutes(1);
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item2_b1_stale));

  PrefetchItem item3_b2_fresh(
      item_generator()->CreateItem(kBucket2StatesToExpire[0]));
  item3_b2_fresh.freshness_time =
      fake_now - kBucket2FreshPeriod + base::TimeDelta::FromMinutes(1);
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item3_b2_fresh));
  PrefetchItem item4_b2_stale(
      item_generator()->CreateItem(kBucket2StatesToExpire[0]));
  item4_b2_stale.freshness_time =
      fake_now - kBucket2FreshPeriod - base::TimeDelta::FromMinutes(1);
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item4_b2_stale));

  PrefetchItem item5_b3_fresh(
      item_generator()->CreateItem(kBucket3StatesToExpire[0]));
  item5_b3_fresh.freshness_time =
      fake_now - kBucket3FreshPeriod + base::TimeDelta::FromMinutes(1);
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item5_b3_fresh));
  PrefetchItem item6_b3_stale(
      item_generator()->CreateItem(kBucket3StatesToExpire[0]));
  item6_b3_stale.freshness_time =
      fake_now - kBucket3FreshPeriod - base::TimeDelta::FromMinutes(1);
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item6_b3_stale));

  // Check inserted initial items.
  std::set<PrefetchItem> initial_items{item1_b1_fresh, item2_b1_stale,
                                       item3_b2_fresh, item4_b2_stale,
                                       item5_b3_fresh, item6_b3_stale};
  std::set<PrefetchItem> all_inserted_items;
  EXPECT_EQ(6U, store_util()->GetAllItems(&all_inserted_items));
  EXPECT_EQ(initial_items, all_inserted_items);

  // Execute the expiration task.
  ExpectTaskCompletes(stale_finalizer_task_.get());
  stale_finalizer_task_->Run();
  RunUntilIdle();
  EXPECT_TRUE(stale_finalizer_task_->ran_successfully());

  // Create the expected finished version of the stale items.
  PrefetchItem item2_b1_finished(item2_b1_stale);
  item2_b1_finished.state = PrefetchItemState::FINISHED;
  item2_b1_finished.error_code =
      PrefetchItemErrorCode::BECAME_STALE_WHILE_AT_BUCKET_1;
  PrefetchItem item4_b2_finished(item4_b2_stale);
  item4_b2_finished.state = PrefetchItemState::FINISHED;
  item4_b2_finished.error_code =
      PrefetchItemErrorCode::BECAME_STALE_WHILE_AT_BUCKET_2;
  PrefetchItem item6_b3_finished(item6_b3_stale);
  item6_b3_finished.state = PrefetchItemState::FINISHED;
  item6_b3_finished.error_code =
      PrefetchItemErrorCode::BECAME_STALE_WHILE_AT_BUCKET_3;

  std::set<PrefetchItem> expected_final_items{
      item1_b1_fresh,    item2_b1_finished, item3_b2_fresh,
      item4_b2_finished, item5_b3_fresh,    item6_b3_finished};
  std::set<PrefetchItem> all_items_post_expiration;
  EXPECT_EQ(6U, store_util()->GetAllItems(&all_items_post_expiration));
  EXPECT_EQ(expected_final_items, all_items_post_expiration);
}

}  // namespace offline_pages
