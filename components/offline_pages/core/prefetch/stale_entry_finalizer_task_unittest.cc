// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/stale_entry_finalizer_task.h"

#include <set>
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
#include "components/offline_pages/core/prefetch/stale_entry_finalizer_delegate_impl.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_test_util.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_utils.h"
#include "components/offline_pages/core/prefetch/task_test_base.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

class TestStaleEntryFinalizerDelegate : public StaleEntryFinalizerDelegateImpl {
 public:
  base::Time Now() override {
    return base::Time() + base::TimeDelta::FromDays(100);
  }
};

class StaleEntryFinalizerTaskTest : public TaskTestBase {
 public:
  StaleEntryFinalizerTaskTest() = default;
  ~StaleEntryFinalizerTaskTest() override = default;

  void SetUp() override;
  void TearDown() override;

 protected:
  std::unique_ptr<StaleEntryFinalizerTask> stale_finalizer_task_;
  TestStaleEntryFinalizerDelegate* test_delegate_;
};

void StaleEntryFinalizerTaskTest::SetUp() {
  TaskTestBase::SetUp();
  stale_finalizer_task_ = base::MakeUnique<StaleEntryFinalizerTask>(store());
  auto test_delegate = base::MakeUnique<TestStaleEntryFinalizerDelegate>();
  test_delegate_ = test_delegate.get();
  stale_finalizer_task_->SetDelegateForTesting(std::move(test_delegate));
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

TEST_F(StaleEntryFinalizerTaskTest, DefaultDelegateDataIsConsistent) {
  // All buckets must have at least one expirable state
  const std::vector<PrefetchItemState>& bucket1states =
      test_delegate_->StatesToExpireForBucket(
          StaleEntryFinalizerTask::BUCKET_1);
  EXPECT_LT(0U, bucket1states.size());
  const std::vector<PrefetchItemState>& bucket2states =
      test_delegate_->StatesToExpireForBucket(
          StaleEntryFinalizerTask::BUCKET_2);
  EXPECT_LT(0U, bucket2states.size());
  const std::vector<PrefetchItemState>& bucket3states =
      test_delegate_->StatesToExpireForBucket(
          StaleEntryFinalizerTask::BUCKET_3);
  EXPECT_LT(0U, bucket3states.size());

  const std::size_t unique_states_count =
      bucket1states.size() + bucket2states.size() + bucket3states.size();

  // Listed states among all buckets must be unique.
  std::set<PrefetchItemState> all_expirable_states;
  all_expirable_states.insert(bucket1states.begin(), bucket1states.end());
  all_expirable_states.insert(bucket2states.begin(), bucket2states.end());
  all_expirable_states.insert(bucket3states.begin(), bucket3states.end());
  EXPECT_EQ(unique_states_count, all_expirable_states.size());

  // Each expirable state must map to a unique error code.
  std::set<PrefetchItemErrorCode> all_respective_error_codes;
  for (PrefetchItemState state : all_expirable_states) {
    all_respective_error_codes.insert(
        test_delegate_->ErrorCodeForStaleEntry(state));
  }
  EXPECT_EQ(unique_states_count, all_respective_error_codes.size());

  // All freshness periods should be greater than 0.
  EXPECT_LT(base::TimeDelta::FromMinutes(0),
            test_delegate_->FreshnessPeriodForBucket(
                StaleEntryFinalizerTask::BUCKET_1));
  EXPECT_LT(base::TimeDelta::FromMinutes(0),
            test_delegate_->FreshnessPeriodForBucket(
                StaleEntryFinalizerTask::BUCKET_2));
  EXPECT_LT(base::TimeDelta::FromMinutes(0),
            test_delegate_->FreshnessPeriodForBucket(
                StaleEntryFinalizerTask::BUCKET_3));
}

TEST_F(StaleEntryFinalizerTaskTest, ExpireStaleItemsOnly) {
  const base::Time fake_now = test_delegate_->Now();

  // Insert a fresh and a stale item into each bucket.
  const std::vector<PrefetchItemState>& bucket1states =
      test_delegate_->StatesToExpireForBucket(
          StaleEntryFinalizerTask::BUCKET_1);
  const base::TimeDelta bucket1FreshPeriod =
      test_delegate_->FreshnessPeriodForBucket(
          StaleEntryFinalizerTask::BUCKET_1);
  ASSERT_LT(base::TimeDelta::FromMinutes(1), bucket1FreshPeriod);

  PrefetchItem item1_b1_fresh(item_generator()->CreateItem(bucket1states[0]));
  item1_b1_fresh.freshness_time =
      fake_now - bucket1FreshPeriod + base::TimeDelta::FromMinutes(1);
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item1_b1_fresh));

  PrefetchItem item2_b1_stale(item_generator()->CreateItem(bucket1states[0]));
  item2_b1_stale.freshness_time =
      fake_now - bucket1FreshPeriod - base::TimeDelta::FromMinutes(1);
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item2_b1_stale));

  const std::vector<PrefetchItemState>& bucket2states =
      test_delegate_->StatesToExpireForBucket(
          StaleEntryFinalizerTask::BUCKET_2);
  const base::TimeDelta bucket2FreshPeriod =
      test_delegate_->FreshnessPeriodForBucket(
          StaleEntryFinalizerTask::BUCKET_2);
  ASSERT_LT(base::TimeDelta::FromMinutes(1), bucket2FreshPeriod);

  PrefetchItem item3_b2_fresh(item_generator()->CreateItem(bucket2states[0]));
  item3_b2_fresh.freshness_time =
      fake_now - bucket2FreshPeriod + base::TimeDelta::FromMinutes(1);
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item3_b2_fresh));

  PrefetchItem item4_b2_stale(item_generator()->CreateItem(bucket2states[0]));
  item4_b2_stale.freshness_time =
      fake_now - bucket2FreshPeriod - base::TimeDelta::FromMinutes(1);
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item4_b2_stale));

  const std::vector<PrefetchItemState>& bucket3states =
      test_delegate_->StatesToExpireForBucket(
          StaleEntryFinalizerTask::BUCKET_3);
  const base::TimeDelta bucket3FreshPeriod =
      test_delegate_->FreshnessPeriodForBucket(
          StaleEntryFinalizerTask::BUCKET_3);
  ASSERT_LT(base::TimeDelta::FromMinutes(1), bucket3FreshPeriod);

  PrefetchItem item5_b3_fresh(item_generator()->CreateItem(bucket3states[0]));
  item5_b3_fresh.freshness_time =
      fake_now - bucket3FreshPeriod + base::TimeDelta::FromMinutes(1);
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item5_b3_fresh));

  PrefetchItem item6_b3_stale(item_generator()->CreateItem(bucket3states[0]));
  item6_b3_stale.freshness_time =
      fake_now - bucket3FreshPeriod - base::TimeDelta::FromMinutes(1);
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
      test_delegate_->ErrorCodeForStaleEntry(bucket1states[0]);
  PrefetchItem item4_b2_finished(item4_b2_stale);
  item4_b2_finished.state = PrefetchItemState::FINISHED;
  item4_b2_finished.error_code =
      test_delegate_->ErrorCodeForStaleEntry(bucket2states[0]);
  PrefetchItem item6_b3_finished(item6_b3_stale);
  item6_b3_finished.state = PrefetchItemState::FINISHED;
  item6_b3_finished.error_code =
      test_delegate_->ErrorCodeForStaleEntry(bucket3states[0]);

  std::set<PrefetchItem> expected_final_items{
      item1_b1_fresh,    item2_b1_finished, item3_b2_fresh,
      item4_b2_finished, item5_b3_fresh,    item6_b3_finished};
  std::set<PrefetchItem> all_items_post_expiration;
  EXPECT_EQ(6U, store_util()->GetAllItems(&all_items_post_expiration));
  EXPECT_EQ(expected_final_items, all_items_post_expiration);
}

}  // namespace offline_pages
