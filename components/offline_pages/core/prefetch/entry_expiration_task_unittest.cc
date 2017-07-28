// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/entry_expiration_task.h"

#include <string>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_test_util.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

class EntryExpirationTaskTest : public testing::Test {
 public:
  EntryExpirationTaskTest();
  ~EntryExpirationTaskTest() override = default;

  void SetUp() override;
  void TearDown() override;

  PrefetchStore* store() { return store_test_util_.store(); }

  PrefetchStoreTestUtil* store_util() { return &store_test_util_; }

  PrefetchItemGenerator* item_generator() { return &item_generator_; }

  void PumpLoop();

 protected:
  std::unique_ptr<EntryExpirationTask> expiration_task_;

 private:
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  PrefetchStoreTestUtil store_test_util_;
  PrefetchItemGenerator item_generator_;
};

EntryExpirationTaskTest::EntryExpirationTaskTest()
    : task_runner_(new base::TestSimpleTaskRunner),
      task_runner_handle_(task_runner_) {}

void EntryExpirationTaskTest::SetUp() {
  store_test_util_.BuildStoreInMemory();

  expiration_task_ = base::MakeUnique<EntryExpirationTask>(store());
}

void EntryExpirationTaskTest::TearDown() {
  ASSERT_FALSE(task_runner_->HasPendingTask());
  expiration_task_.reset();

  store_test_util_.DeleteStore();
  PumpLoop();
}

void EntryExpirationTaskTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

PrefetchStore::ResultCallback<bool> EntryExpirationTaskTest::ExpectTrueCallback() {
  return base::BindOnce(&EntryExpirationTaskTest::ExpectTrue);
}

TEST_F(EntryExpirationTaskTest, ExpireStaleItems) {
  base::Time fake_now = base::Time() + base::TimeDelta::FromDays(100);
  PrefetchItem item1_b1_fresh(item_generator()->CreateItem(kBucket1States[0]));
  item1_b1_fresh.freshness_time = fake_now - kBucket1FreshPeriod + base::TimeDelta::FromMinutes(1);
  store_util()->InsertItem(item1_b1_fresh, ExpectTrueCallback());
  PumpLoop();
  PrefetchItem item2_b1_stale(item_generator()->CreateItem(kBucket1States[0]));
  item2_b1_stale.freshness_time = fake_now - kBucket1FreshPeriod - base::TimeDelta::FromMinutes(1);
  store_util()->InsertItem(item2_b1_stale, ExpectTrueCallback());

  PrefetchItem item3_b2_fresh(item_generator()->CreateItem(kBucket2States[0]));
  item3_b2_fresh.freshness_time = fake_now - kBucket2FreshPeriod + base::TimeDelta::FromMinutes(1);
  store_util()->InsertItem(item3_b2_fresh, ExpectTrueCallback());
  PrefetchItem item4_b2_stale(item_generator()->CreateItem(kBucket2States[0]));
  item4_b2_stale.freshness_time = fake_now - kBucket2FreshPeriod - base::TimeDelta::FromMinutes(1);
  store_util()->InsertItem(item4_b2_stale, ExpectTrueCallback());

  PrefetchItem item5_b3_fresh(item_generator()->CreateItem(kBucket3States[0]));
  item5_b3_fresh.freshness_time = fake_now - kBucket3FreshPeriod + base::TimeDelta::FromMinutes(1);
  store_util()->InsertItem(item5_b3_fresh, ExpectTrueCallback());
  PrefetchItem item6_b3_stale(item_generator()->CreateItem(kBucket3States[0]));
  item6_b3_stale.freshness_time = fake_now - kBucket3FreshPeriod - base::TimeDelta::FromMinutes(1);
  store_util()->InsertItem(item6_b3_stale, ExpectTrueCallback());
  PumpLoop();

  std::set<PrefetchItem> all_items;
  store_util()->GetAllItems(&all_items, ExpectEqCallback<std::size_t>(6U));
  PumpLoop();

  std::set<int64_t> all_stale_offline_ids{item2_b1_stale.offline_id, item4_b2_stale.offline_id, item6_b3_stale.offline_id};
  ASSERT_EQ(3U, all_stale_offline_ids.size());
  std::set<PrefetchItem> finished_items = FilterItemsByState(all_items, PrefetchItemState::FINISHED);
  for (const PrefetchItem& finished_item : finished_items) {
    EXPECT_EQ(PrefetchItemErrorCode::EXPIRED, finished_item.error_code);
    EXPECT_TRUE(all_stale_offline_ids.count(finished_item.offline_id));
  }

  std::set<PrefetchItem> still_fresh_items = ItemsDifference(all_items, finished_items);
  EXPECT_TRUE(still_fresh_items.count(item1_b1_fresh));
  EXPECT_TRUE(still_fresh_items.count(item3_b2_fresh));
  EXPECT_TRUE(still_fresh_items.count(item5_b3_fresh));
}

}  // namespace offline_pages
