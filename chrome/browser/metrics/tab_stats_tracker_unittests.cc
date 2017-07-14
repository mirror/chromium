// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/tab_stats_tracker.h"

#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace metrics {

class TabStatsTrackerTest : public InProcessBrowserTest {
 public:
  TabStatsTrackerTest() {}

  void SetUpOnMainThread() override {
    tab_stats_tracker_ = TabStatsTracker::GetInstance();
  }

 protected:
  TabStatsTracker* tab_stats_tracker_;

  DISALLOW_COPY_AND_ASSIGN(TabStatsTrackerTest);
};

IN_PROC_BROWSER_TEST_F(TabStatsTrackerTest,
                       TabsAndBrowsersAreCountedAccurately) {
  ASSERT_NE(static_cast<TabStatsTracker*>(nullptr), tab_stats_tracker_);
  EXPECT_EQ(1, tab_stats_tracker_->browser_count());
  EXPECT_EQ(1, tab_stats_tracker_->total_tab_count());

  GURL url1("data:text/html,page1");
  AddTabAtIndex(1, url1, ui::PAGE_TRANSITION_TYPED);
  EXPECT_EQ(2, tab_stats_tracker_->total_tab_count());

  browser()->tab_strip_model()->CloseWebContentsAt(1, 0);
  EXPECT_EQ(1, tab_stats_tracker_->total_tab_count());

  Browser* browser = CreateBrowser(ProfileManager::GetActiveUserProfile());
  EXPECT_EQ(2, tab_stats_tracker_->browser_count());

  AddTabAtIndexToBrowser(browser, 1, url1, ui::PAGE_TRANSITION_TYPED, true);
}

}  // namespace metrics
