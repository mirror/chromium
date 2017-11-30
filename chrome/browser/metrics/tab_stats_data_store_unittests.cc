// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/tab_stats_data_store.h"

#include "base/test/scoped_task_environment.h"
#include "chrome/browser/metrics/tab_stats_tracker.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/browser/web_contents.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace metrics {

namespace {

using TabsStats = TabStatsDataStore::TabsStats;

class TabStatsDataStoreTest : public ChromeRenderViewHostTestHarness {
 public:
  TabStatsDataStoreTest() {
    TabStatsTracker::RegisterPrefs(pref_service_.registry());
    data_store_ = std::make_unique<TabStatsDataStore>(&pref_service_);
  }

 protected:
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<TabStatsDataStore> data_store_;
};

}  // namespace

TEST_F(TabStatsDataStoreTest, TabStatsGetsReloadedFromLocalState) {
  // This tests creates add some tabs/windows to a data store instance and then
  // reinitializes it (and so the count of active tabs/windows drops to zero).
  // As the TabStatsTracker constructor restores its state from the pref service
  // the maximums should be restored.
  size_t expected_tab_count = 12;
  std::vector<std::unique_ptr<content::WebContents>> test_web_contents_vec;
  for (size_t i = 0; i < expected_tab_count; ++i) {
    std::unique_ptr<content::WebContents> test_web_contents =
        base::WrapUnique(CreateTestWebContents());
    data_store_->OnTabAdded(test_web_contents.get());
    test_web_contents_vec.push_back(std::move(test_web_contents));
  }
  size_t expected_window_count = 5;
  for (size_t i = 0; i < expected_window_count; ++i)
    data_store_->OnWindowAdded();
  size_t expected_max_tab_per_window = expected_tab_count - 1;
  data_store_->UpdateMaxTabsPerWindowIfNeeded(expected_max_tab_per_window);

  TabsStats stats = data_store_->tab_stats();
  EXPECT_EQ(expected_tab_count, stats.total_tab_count_max);
  EXPECT_EQ(expected_max_tab_per_window, stats.max_tab_per_window);
  EXPECT_EQ(expected_window_count, stats.window_count_max);

  // Reset the |tab_stats_tracker_| and ensure that the maximums are restored.
  data_store_.reset(new TabStatsDataStore(&pref_service_));

  TabsStats stats2 = data_store_->tab_stats();
  EXPECT_EQ(stats.total_tab_count_max, stats2.total_tab_count_max);
  EXPECT_EQ(stats.max_tab_per_window, stats2.max_tab_per_window);
  EXPECT_EQ(stats.window_count_max, stats2.window_count_max);
  // The actual number of tabs/window should be 0 as it's not stored in the
  // pref service.
  EXPECT_EQ(0U, stats2.window_count);
  EXPECT_EQ(0U, stats2.total_tab_count);
}

TEST_F(TabStatsDataStoreTest, TrackTabUsageDuringInterval) {
  const size_t kTestIntervalLength = 42;

  std::vector<std::unique_ptr<content::WebContents>> web_contents_vec;
  for (size_t i = 0; i < 3; ++i) {
    web_contents_vec.push_back(base::WrapUnique(CreateTestWebContents()));
    // Make sure that these WebContents are initially not visible.
    web_contents_vec.back()->WasHidden();
  }

  // Creates a test interval.
  data_store_->AddInterval(kTestIntervalLength);
  TabStatsDataStore::TabsStateDuringIntervalMap* interval_map =
      data_store_->GetIntervalMap(kTestIntervalLength);
  EXPECT_EQ(0U, interval_map->size());

  // Adds the tabs to the data store.
  for (auto& iter : web_contents_vec)
    data_store_->OnTabAdded(iter.get());

  EXPECT_EQ(web_contents_vec.size(), interval_map->size());

  for (const auto& iter : web_contents_vec) {
    auto map_iter = interval_map->find(iter.get());
    EXPECT_TRUE(map_iter != interval_map->end());

    // The tabs have been inserted after the creation of the interval.
    EXPECT_FALSE(map_iter->second.existed_before_interval);
    EXPECT_FALSE(map_iter->second.visible_during_interval);
    EXPECT_FALSE(map_iter->second.interacted_during_interval);
    EXPECT_TRUE(map_iter->second.exists_after_interval);
  }

  // Mark a tab as active, this count as an interaction.
  data_store_->SetTabActive(web_contents_vec[0].get());
  EXPECT_TRUE(interval_map->find(web_contents_vec[0].get())
                  ->second.interacted_during_interval);
  EXPECT_FALSE(interval_map->find(web_contents_vec[1].get())
                   ->second.interacted_during_interval);
  EXPECT_FALSE(interval_map->find(web_contents_vec[2].get())
                   ->second.interacted_during_interval);

  data_store_->OnTabInteraction(web_contents_vec[1].get());
  EXPECT_TRUE(interval_map->find(web_contents_vec[0].get())
                  ->second.interacted_during_interval);
  EXPECT_TRUE(interval_map->find(web_contents_vec[1].get())
                  ->second.interacted_during_interval);
  EXPECT_FALSE(interval_map->find(web_contents_vec[2].get())
                   ->second.interacted_during_interval);

  data_store_->SetTabUnactive(web_contents_vec[0].get());
  // Marking the tab as unactive shouldn't change the
  // |interacted_during_interval| value until the interval gets reset.
  EXPECT_TRUE(interval_map->find(web_contents_vec[0].get())
                  ->second.interacted_during_interval);
  EXPECT_TRUE(interval_map->find(web_contents_vec[1].get())
                  ->second.interacted_during_interval);
  EXPECT_FALSE(interval_map->find(web_contents_vec[2].get())
                   ->second.interacted_during_interval);

  data_store_->ResetIntervalData(interval_map);
  EXPECT_FALSE(interval_map->find(web_contents_vec[0].get())
                   ->second.interacted_during_interval);
  EXPECT_FALSE(interval_map->find(web_contents_vec[1].get())
                   ->second.interacted_during_interval);
  EXPECT_FALSE(interval_map->find(web_contents_vec[2].get())
                   ->second.interacted_during_interval);

  data_store_->OnTabVisible(web_contents_vec[0].get());
  EXPECT_TRUE(interval_map->find(web_contents_vec[0].get())
                  ->second.visible_during_interval);
  EXPECT_FALSE(interval_map->find(web_contents_vec[1].get())
                   ->second.visible_during_interval);
  EXPECT_FALSE(interval_map->find(web_contents_vec[2].get())
                   ->second.visible_during_interval);
}

}  // namespace metrics
