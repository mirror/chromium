// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/tab_stats_tracker.h"

#include "base/message_loop/message_loop.h"
#include "base/test/histogram_tester.h"
#include "base/test/power_monitor_test_base.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using TabsStatsDataStore = metrics::TabsStatsDataStore;
using TabsStats = TabsStatsDataStore::TabsStats;
using TabStatsTracker = metrics::TabStatsTracker;

class TabStatsTrackerBrowserTest : public InProcessBrowserTest {
 public:
  TabStatsTrackerBrowserTest() : tab_stats_tracker_(nullptr) {}

  void SetUpOnMainThread() override {
    EXPECT_NE(nullptr, g_browser_process->local_state());
    TabStatsTracker::Initialize(g_browser_process->local_state());
    tab_stats_tracker_ = TabStatsTracker::GetInstance();
    ASSERT_NE(nullptr, tab_stats_tracker_);
  }

 protected:
  TabStatsTracker* tab_stats_tracker_;

  DISALLOW_COPY_AND_ASSIGN(TabStatsTrackerBrowserTest);
};

class TestTabStatsTracker : public TabStatsTracker {
 public:
  using UmaStatsReportingDelegate = TabStatsTracker::UmaStatsReportingDelegate;

  explicit TestTabStatsTracker(PrefService* pref_service)
      : TabStatsTracker(pref_service), pref_service_(pref_service) {}
  ~TestTabStatsTracker() override {}

  size_t AddTabs(size_t tab_count) {
    tab_stats_data_store_->OnTabsAdded(tab_count);
    return tab_stats_data_store_->tab_stats().total_tab_count;
  }

  size_t RemoveTabs(size_t tab_count) {
    EXPECT_LE(tab_count, tab_stats_data_store_->tab_stats().total_tab_count);
    tab_stats_data_store_->OnTabsRemoved(tab_count);
    return tab_stats_data_store_->tab_stats().total_tab_count;
  }

  size_t AddBrowsers(size_t browser_count) {
    tab_stats_data_store_->OnBrowsersAdded(browser_count);
    return tab_stats_data_store_->tab_stats().browser_count;
  }

  size_t RemoveBrowsers(size_t browser_count) {
    EXPECT_LE(browser_count, tab_stats_data_store_->tab_stats().browser_count);
    tab_stats_data_store_->OnBrowsersRemoved(browser_count);
    return tab_stats_data_store_->tab_stats().browser_count;
  }

  void CheckDailyEventInterval() { daily_event_->CheckInterval(); }

  void ResetDailyEvent() {
    daily_event_.reset(new metrics::DailyEvent(
        pref_service_, metrics::prefs::kTabStatsDailySample,
        kTabStatsDailyEventHistogramName));
    daily_event_->AddObserver(base::MakeUnique<TabStatsDailyObserver>(
        reporting_delegate_.get(), tab_stats_data_store_.get()));
  }

  TabsStatsDataStore* tab_stats_data_store() {
    return tab_stats_data_store_.get();
  }

  base::Timer* timer() { return &timer_; }

 private:
  PrefService* pref_service_;

  DISALLOW_COPY_AND_ASSIGN(TestTabStatsTracker);
};

class TabStatsTrackerTest : public testing::Test {
 public:
  using UmaStatsReportingDelegate =
      TestTabStatsTracker::UmaStatsReportingDelegate;

  TabStatsTrackerTest() {
    power_monitor_source_ = new base::PowerMonitorTestSource();
    power_monitor_.reset(new base::PowerMonitor(
        std::unique_ptr<base::PowerMonitorSource>(power_monitor_source_)));

    pref_service_.registry()->RegisterIntegerPref(
        metrics::prefs::kTabStatsTotalTabCountMax, 0);
    pref_service_.registry()->RegisterIntegerPref(
        metrics::prefs::kTabStatsMaxTabsPerWindow, 0);
    pref_service_.registry()->RegisterIntegerPref(
        metrics::prefs::kTabStatsBrowserCountMax, 0);
    metrics::DailyEvent::RegisterPref(pref_service_.registry(),
                                      metrics::prefs::kTabStatsDailySample);
    // The tab stats tracker has to be created after the power monitor has it's
    // using it.
    tab_stats_tracker_.reset(new TestTabStatsTracker(&pref_service_));
  }

  void TearDown() override { tab_stats_tracker_.reset(nullptr); }

  // The tabs stat tracker instance, it should be created in the SetUp
  std::unique_ptr<TestTabStatsTracker> tab_stats_tracker_;

  // Used to simulate power events.
  base::PowerMonitorTestSource* power_monitor_source_;
  std::unique_ptr<base::PowerMonitor> power_monitor_;

  // The Power Monitor requires a message loop.
  base::MessageLoop message_loop_;

  // Used to make sure that the metrics are reported properly.
  base::HistogramTester histogram_tester_;

  TestingPrefServiceSimple pref_service_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TabStatsTrackerTest);
};

// Comparator for base::Bucket values.
bool CompareHistogramBucket(const base::Bucket& l, const base::Bucket& r) {
  return l.min < r.min;
}

bool TabStatsMatchExpectations(const TabsStats& expected,
                               const TabsStats& actual) {
  EXPECT_EQ(actual.total_tab_count, expected.total_tab_count);
  EXPECT_EQ(actual.total_tab_count_max, expected.total_tab_count_max);
  EXPECT_EQ(actual.max_tab_per_window, expected.max_tab_per_window);
  EXPECT_EQ(actual.browser_count, expected.browser_count);
  EXPECT_EQ(actual.browser_count_max, expected.browser_count_max);

  return ::memcmp(&expected, &actual, sizeof(TabsStats)) == 0;
}

}  // namespace

IN_PROC_BROWSER_TEST_F(TabStatsTrackerBrowserTest,
                       TabsAndBrowsersAreCountedAccurately) {
  // Assert that the |TabStatsTracker| instance is initialized during the
  // creation of the main browser.
  ASSERT_NE(static_cast<TabStatsTracker*>(nullptr), tab_stats_tracker_);

  TabsStats expected_stats = {};

  // There should be only one windows with one tab at startup.
  expected_stats.total_tab_count = 1;
  expected_stats.total_tab_count_max = 1;
  expected_stats.max_tab_per_window = 1;
  expected_stats.browser_count = 1;
  expected_stats.browser_count_max = 1;

  EXPECT_TRUE(TabStatsMatchExpectations(expected_stats,
                                        tab_stats_tracker_->tab_stats()));

  // Add a tab and make sure that the counters get updated.
  AddTabAtIndex(1, GURL("about:blank"), ui::PAGE_TRANSITION_TYPED);
  ++expected_stats.total_tab_count;
  ++expected_stats.total_tab_count_max;
  ++expected_stats.max_tab_per_window;
  EXPECT_TRUE(TabStatsMatchExpectations(expected_stats,
                                        tab_stats_tracker_->tab_stats()));

  browser()->tab_strip_model()->CloseWebContentsAt(1, 0);
  --expected_stats.total_tab_count;
  EXPECT_TRUE(TabStatsMatchExpectations(expected_stats,
                                        tab_stats_tracker_->tab_stats()));

  Browser* browser = CreateBrowser(ProfileManager::GetActiveUserProfile());
  ++expected_stats.total_tab_count;
  ++expected_stats.browser_count;
  ++expected_stats.browser_count_max;
  EXPECT_TRUE(TabStatsMatchExpectations(expected_stats,
                                        tab_stats_tracker_->tab_stats()));

  AddTabAtIndexToBrowser(browser, 1, GURL("about:blank"),
                         ui::PAGE_TRANSITION_TYPED, true);
  ++expected_stats.total_tab_count;
  ++expected_stats.total_tab_count_max;
  EXPECT_TRUE(TabStatsMatchExpectations(expected_stats,
                                        tab_stats_tracker_->tab_stats()));

  CloseBrowserSynchronously(browser);
  expected_stats.total_tab_count = 1;
  expected_stats.browser_count = 1;
  EXPECT_TRUE(TabStatsMatchExpectations(expected_stats,
                                        tab_stats_tracker_->tab_stats()));
}

TEST_F(TabStatsTrackerTest, OnResume) {
  // Makes sure that there's no sample initially.
  histogram_tester_.ExpectTotalCount(
      UmaStatsReportingDelegate::kNumberOfTabsOnResumeHistogramName, 0);

  // Creates some tabs.
  size_t expected_tab_count = tab_stats_tracker_->AddTabs(12);

  std::vector<base::Bucket> count_buckets;
  count_buckets.push_back(base::Bucket(expected_tab_count, 1));

  // Generates a resume event that should end up calling the
  // |ReportTabCountOnResume| method of the reporting delegate.
  power_monitor_source_->GenerateSuspendEvent();
  power_monitor_source_->GenerateResumeEvent();

  // There should be only one sample for the |kNumberOfTabsOnResume| histogram.
  histogram_tester_.ExpectTotalCount(
      UmaStatsReportingDelegate::kNumberOfTabsOnResumeHistogramName,
      count_buckets.size());
  EXPECT_EQ(histogram_tester_.GetAllSamples(
                UmaStatsReportingDelegate::kNumberOfTabsOnResumeHistogramName),
            count_buckets);

  // Removes some tabs and update the expectations.
  expected_tab_count = tab_stats_tracker_->RemoveTabs(5);
  count_buckets.push_back(base::Bucket(expected_tab_count, 1));
  std::sort(count_buckets.begin(), count_buckets.end(), CompareHistogramBucket);

  // Generates another resume event.
  power_monitor_source_->GenerateSuspendEvent();
  power_monitor_source_->GenerateResumeEvent();

  // There should be 2 samples for this metric now.
  histogram_tester_.ExpectTotalCount(
      UmaStatsReportingDelegate::kNumberOfTabsOnResumeHistogramName,
      count_buckets.size());
  EXPECT_EQ(histogram_tester_.GetAllSamples(
                UmaStatsReportingDelegate::kNumberOfTabsOnResumeHistogramName),
            count_buckets);
}

TEST_F(TabStatsTrackerTest, TabStatsGetsReloadedFromLocalState) {
  size_t expected_tab_count = tab_stats_tracker_->AddTabs(12);
  size_t expected_browser_count = tab_stats_tracker_->AddBrowsers(5);
  tab_stats_tracker_->tab_stats_data_store()->UpdateMaxTabsPerWindowIfNeeded(
      expected_tab_count);

  TabsStats stats = tab_stats_tracker_->tab_stats_data_store()->tab_stats();
  EXPECT_EQ(expected_tab_count, stats.total_tab_count_max);
  EXPECT_EQ(expected_tab_count, stats.max_tab_per_window);
  EXPECT_EQ(expected_browser_count, stats.browser_count_max);

  tab_stats_tracker_.reset(new TestTabStatsTracker(&pref_service_));

  TabsStats stats2 = tab_stats_tracker_->tab_stats_data_store()->tab_stats();
  EXPECT_EQ(stats.total_tab_count_max, stats2.total_tab_count_max);
  EXPECT_EQ(stats.max_tab_per_window, stats2.max_tab_per_window);
  EXPECT_EQ(stats.browser_count_max, stats2.browser_count_max);
  EXPECT_EQ(0U, stats2.browser_count);
  EXPECT_EQ(0U, stats2.total_tab_count);
}

TEST_F(TabStatsTrackerTest, StatsGetReportedDaily) {
  EXPECT_TRUE(tab_stats_tracker_->timer()->IsRunning());
  tab_stats_tracker_->timer()->Stop();

  size_t expected_tab_count = tab_stats_tracker_->AddTabs(12);
  size_t expected_browser_count = tab_stats_tracker_->AddBrowsers(5);
  tab_stats_tracker_->tab_stats_data_store()->UpdateMaxTabsPerWindowIfNeeded(
      expected_tab_count);
  expected_tab_count = tab_stats_tracker_->RemoveTabs(5);
  expected_browser_count = tab_stats_tracker_->RemoveBrowsers(2);

  TabsStats stats = tab_stats_tracker_->tab_stats_data_store()->tab_stats();

  base::Time last_time = base::Time::Now() - base::TimeDelta::FromHours(25);
  pref_service_.SetInt64(metrics::prefs::kTabStatsDailySample,
                         last_time.since_origin().InMicroseconds());
  tab_stats_tracker_->CheckDailyEventInterval();

  EXPECT_NE(last_time.since_origin().InMicroseconds(),
            pref_service_.GetInt64(metrics::prefs::kTabStatsDailySample));

  histogram_tester_.ExpectUniqueSample(
      UmaStatsReportingDelegate::kMaxTabsInADayHistogramName,
      stats.total_tab_count_max, 1);
  histogram_tester_.ExpectUniqueSample(
      UmaStatsReportingDelegate::kMaxTabsPerBrowserInADayHistogramName,
      stats.max_tab_per_window, 1);
  histogram_tester_.ExpectUniqueSample(
      UmaStatsReportingDelegate::kMaxBrowsersInADayHistogramName,
      stats.browser_count_max, 1);

  tab_stats_tracker_->tab_stats_data_store()->UpdateMaxTabsPerWindowIfNeeded(
      expected_tab_count);

  stats = tab_stats_tracker_->tab_stats_data_store()->tab_stats();
  EXPECT_EQ(expected_tab_count, stats.total_tab_count_max);
  EXPECT_EQ(expected_tab_count, stats.max_tab_per_window);
  EXPECT_EQ(expected_browser_count, stats.browser_count_max);
  EXPECT_EQ(expected_tab_count, pref_service_.GetInteger(
                                    metrics::prefs::kTabStatsTotalTabCountMax));
  EXPECT_EQ(expected_tab_count, pref_service_.GetInteger(
                                    metrics::prefs::kTabStatsMaxTabsPerWindow));
  EXPECT_EQ(expected_browser_count,
            pref_service_.GetInteger(metrics::prefs::kTabStatsBrowserCountMax));

  tab_stats_tracker_->ResetDailyEvent();
  pref_service_.SetInt64(metrics::prefs::kTabStatsDailySample,
                         last_time.since_origin().InMicroseconds());
  tab_stats_tracker_->CheckDailyEventInterval();

  histogram_tester_.ExpectBucketCount(
      UmaStatsReportingDelegate::kMaxTabsInADayHistogramName,
      stats.total_tab_count_max, 1);
  histogram_tester_.ExpectBucketCount(
      UmaStatsReportingDelegate::kMaxTabsPerBrowserInADayHistogramName,
      stats.max_tab_per_window, 1);
  histogram_tester_.ExpectBucketCount(
      UmaStatsReportingDelegate::kMaxBrowsersInADayHistogramName,
      stats.browser_count_max, 1);
}
