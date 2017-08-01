// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/tab_stats_tracker.h"

#include "base/message_loop/message_loop.h"
#include "base/test/histogram_tester.h"
#include "base/test/power_monitor_test_base.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using TabStatsTracker = metrics::TabStatsTracker;
using StatsReportingDelegate = TabStatsTracker::StatsReportingDelegate;

class TabStatsTrackerBrowserTest : public InProcessBrowserTest {
 public:
  TabStatsTrackerBrowserTest() : tab_stats_tracker_(nullptr) {}

  void SetUpOnMainThread() override {
    tab_stats_tracker_ = TabStatsTracker::GetInstance();
  }

 protected:
  TabStatsTracker* tab_stats_tracker_;

  DISALLOW_COPY_AND_ASSIGN(TabStatsTrackerBrowserTest);
};

}  // namespace

IN_PROC_BROWSER_TEST_F(TabStatsTrackerBrowserTest,
                       TabsAndBrowsersAreCountedAccurately) {
  // Assert that the |TabStatsTracker| instance is initialized during the
  // creation of the main browser.
  ASSERT_NE(static_cast<TabStatsTracker*>(nullptr), tab_stats_tracker_);

  // There should be only one windows with one tab at startup.
  EXPECT_EQ(1U, tab_stats_tracker_->browser_count());
  EXPECT_EQ(1U, tab_stats_tracker_->total_tab_count());

  // Add a tab and make sure that the counters get updated.
  AddTabAtIndex(1, GURL("about:blank"), ui::PAGE_TRANSITION_TYPED);
  EXPECT_EQ(2U, tab_stats_tracker_->total_tab_count());
  browser()->tab_strip_model()->CloseWebContentsAt(1, 0);
  EXPECT_EQ(1U, tab_stats_tracker_->total_tab_count());

  Browser* browser = CreateBrowser(ProfileManager::GetActiveUserProfile());
  EXPECT_EQ(2U, tab_stats_tracker_->browser_count());

  AddTabAtIndexToBrowser(browser, 1, GURL("about:blank"),
                         ui::PAGE_TRANSITION_TYPED, true);
  EXPECT_EQ(3U, tab_stats_tracker_->total_tab_count());

  CloseBrowserSynchronously(browser);
  EXPECT_EQ(1U, tab_stats_tracker_->browser_count());
  EXPECT_EQ(1U, tab_stats_tracker_->total_tab_count());
}

namespace {

class TestTabStatsTracker : public TabStatsTracker {
 public:
  TestTabStatsTracker() {}
  ~TestTabStatsTracker() override {}

  size_t AddTabs(size_t tab_count) {
    total_tabs_count_ += tab_count;
    return total_tabs_count_;
  }

  size_t RemoveTabs(size_t tab_count) {
    EXPECT_LE(tab_count, total_tabs_count_);
    total_tabs_count_ -= tab_count;
    return total_tabs_count_;
  }

  size_t AddBrowsers(size_t browser_count) {
    browser_count_ += browser_count;
    return browser_count_;
  }

  size_t RemoveBrowsers(size_t browser_count) {
    EXPECT_LE(browser_count, browser_count_);
    browser_count_ -= browser_count;
    return browser_count_;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestTabStatsTracker);
};

class TabStatsTrackerTest : public testing::Test {
 public:
  TabStatsTrackerTest() {
    power_monitor_source_ = new base::PowerMonitorTestSource();
    power_monitor_.reset(new base::PowerMonitor(
        std::unique_ptr<base::PowerMonitorSource>(power_monitor_source_)));
  }

  void SetUp() override { tab_stats_tracker_.reset(new TestTabStatsTracker()); }

  void TearDown() override { tab_stats_tracker_.reset(nullptr); }

  std::unique_ptr<TestTabStatsTracker> tab_stats_tracker_;
  base::PowerMonitorTestSource* power_monitor_source_;
  std::unique_ptr<base::PowerMonitor> power_monitor_;
  base::HistogramTester histogram_tester_;
  base::MessageLoop message_loop_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TabStatsTrackerTest);
};

}  // namespace

TEST_F(TabStatsTrackerTest, OnResume) {
  histogram_tester_.ExpectTotalCount("TabStats.NumberOfTabsOnResume", 0);
  size_t expected_tab_count = tab_stats_tracker_->AddTabs(12);
  expected_tab_count = tab_stats_tracker_->RemoveTabs(5);

  power_monitor_source_->GenerateSuspendEvent();
  power_monitor_source_->GenerateResumeEvent();
  histogram_tester_.ExpectTotalCount("TabStats.NumberOfTabsOnResume", 1);
}
