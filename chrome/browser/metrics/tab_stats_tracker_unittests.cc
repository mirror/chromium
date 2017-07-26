// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/tab_stats_tracker.h"

#include "base/message_loop/message_loop.h"
#include "base/test/power_monitor_test_base.h"
#include "base/test/histogram_tester.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using TabStatsTracker = metrics::TabStatsTracker;
using StatsReportingDelegate = TabStatsTracker::StatsReportingDelegate;

// A mock StatsReportingDelegate. This is used by the unittests to validate the
// reporting and lifetime behaviour of the TabStatsTracker under test.
class MockStatsReportingDelegate : public StatsReportingDelegate {
 public:
  MockStatsReportingDelegate()
      : report_tab_at_resume_count_(0u) {}

  ~MockStatsReportingDelegate() override { EnsureNoUnexpectedCalls(); }

  void ReportTabCountOnResume(size_t tab_count) override {
    report_tab_at_resume_count_ = tab_count;
  }

  void ExpectReportTabAtResumeMatchCount(size_t tab_count) {
    EXPECT_EQ(tab_count, report_tab_at_resume_count_);
  }

  void EnsureNoUnexpectedCalls() {
    EXPECT_EQ(0u, report_tab_at_resume_count_);
  }

 private:
  size_t report_tab_at_resume_count_;

  DISALLOW_COPY_AND_ASSIGN(MockStatsReportingDelegate);
};

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

class TabStatsTrackerTest : public testing::Test {
 public:
  TabStatsTrackerTest() {
    power_monitor_source_ = new base::PowerMonitorTestSource();
    power_monitor_.reset(new base::PowerMonitor(
        std::unique_ptr<base::PowerMonitorSource>(power_monitor_source_)));
  };

  base::PowerMonitorTestSource* power_monitor_source_;
  std::unique_ptr<base::PowerMonitor> power_monitor_;
  base::HistogramTester histogram_tester_;
  base::MessageLoop message_loop_;
 private:
  DISALLOW_COPY_AND_ASSIGN(TabStatsTrackerTest);
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

TEST_F(TabStatsTrackerTest, OnResume) {
  metrics::TabStatsTracker::Initialize(base::MakeUnique<
      metrics::TabStatsTracker::UmaStatsReportingDelegate>());

  power_monitor_source_->GenerateResumeEvent();
  histogram_tester_.ExpectTotalCount("TabStats.NumberOfTabsOnResume", 1);
}
