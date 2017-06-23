// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/prefetch/offline_metrics_collector_impl.h"

#include <memory>
#include <string>

#include "base/test/histogram_tester.h"
#include "base/test/simple_test_clock.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace offline_pages {
using UsageType = OfflineMetricsCollectorImpl::UsageType;

class OfflineMetricsCollectorTest : public testing::Test {
 public:
  OfflineMetricsCollectorTest() {}
  ~OfflineMetricsCollectorTest() override {}

  // testing::Test:
  void SetUp() override {
    test_clock_ = base::MakeUnique<base::SimpleTestClock>();
    test_clock_->SetNow(base::Time::Now().LocalMidnight());
    pref_service_ = base::MakeUnique<TestingPrefServiceSimple>();
    OfflineMetricsCollectorImpl::RegisterPrefs(pref_service_->registry());
    Reload();
  }

  // This creates new collector whcih will read the initial values from Prefs.
  void Reload() {
    collector_ =
        base::MakeUnique<OfflineMetricsCollectorImpl>(pref_service_.get());
    collector_->SetClockForTesting(test_clock());
  }

  base::SimpleTestClock* test_clock() { return test_clock_.get(); }
  PrefService* prefs() const { return pref_service_.get(); }
  OfflineMetricsCollector* collector() const { return collector_.get(); }
  const base::HistogramTester& histograms() const { return histogram_tester_; }

  UsageType GetAccumulatedUsageFromPrefs() {
    return static_cast<UsageType>(
        prefs()->GetInteger(prefs::kOfflineAccumulatedUsage));
  }

  base::Time GetTimestampFromPrefs() {
    return base::Time::FromInternalValue(
        pref_service_->GetInt64(prefs::kOfflineAccumulatedTimestamp));
  }

 protected:
  std::unique_ptr<base::SimpleTestClock> test_clock_;
  std::unique_ptr<TestingPrefServiceSimple> pref_service_;
  std::unique_ptr<OfflineMetricsCollectorImpl> collector_;
  base::HistogramTester histogram_tester_;

  DISALLOW_COPY_AND_ASSIGN(OfflineMetricsCollectorTest);
};

TEST_F(OfflineMetricsCollectorTest, CheckCleanInit) {
  EXPECT_EQ(0L, prefs()->GetInt64(prefs::kOfflineAccumulatedTimestamp));
  EXPECT_EQ(0, prefs()->GetInteger(prefs::kOfflineAccumulatedUsage));
  EXPECT_EQ(0, prefs()->GetInteger(prefs::kOfflineUsageUnusedCount));
  EXPECT_EQ(0, prefs()->GetInteger(prefs::kOfflineUsageStartedCount));
  EXPECT_EQ(0, prefs()->GetInteger(prefs::kOfflineUsageOfflineCount));
  EXPECT_EQ(0, prefs()->GetInteger(prefs::kOfflineUsageOnlineCount));
  EXPECT_EQ(0, prefs()->GetInteger(prefs::kOfflineUsageMixedCount));
  EXPECT_EQ(UsageType::UNUSED, GetAccumulatedUsageFromPrefs());
}

TEST_F(OfflineMetricsCollectorTest, FirstStart) {
  EXPECT_EQ(UsageType::UNUSED, GetAccumulatedUsageFromPrefs());
  base::Time start = test_clock()->Now();

  collector()->OnAppStartupOrResume();

  EXPECT_EQ(UsageType::STARTED, GetAccumulatedUsageFromPrefs());
  // Timestamp shoudln't change.
  EXPECT_EQ(GetTimestampFromPrefs(), start);
  // Accumulated counters shouldn't change.
  EXPECT_EQ(0, prefs()->GetInteger(prefs::kOfflineUsageUnusedCount));
  EXPECT_EQ(0, prefs()->GetInteger(prefs::kOfflineUsageStartedCount));
  EXPECT_EQ(0, prefs()->GetInteger(prefs::kOfflineUsageOfflineCount));
  EXPECT_EQ(0, prefs()->GetInteger(prefs::kOfflineUsageOnlineCount));
  EXPECT_EQ(0, prefs()->GetInteger(prefs::kOfflineUsageMixedCount));
}

// Offline + Online -> Mixed
TEST_F(OfflineMetricsCollectorTest, Transitions1) {
  collector()->OnAppStartupOrResume();
  collector()->OnSuccessfulNavigationOffline();
  EXPECT_EQ(UsageType::OFFLINE, GetAccumulatedUsageFromPrefs());
  collector()->OnSuccessfulNavigationOnline();
  EXPECT_EQ(UsageType::MIXED, GetAccumulatedUsageFromPrefs());
}

// Online + Offline -> Mixed
TEST_F(OfflineMetricsCollectorTest, Transitions2) {
  collector()->OnAppStartupOrResume();
  collector()->OnSuccessfulNavigationOnline();
  EXPECT_EQ(UsageType::ONLINE, GetAccumulatedUsageFromPrefs());
  collector()->OnSuccessfulNavigationOffline();
  EXPECT_EQ(UsageType::MIXED, GetAccumulatedUsageFromPrefs());
}

// Mixed is terminal state
TEST_F(OfflineMetricsCollectorTest, MixedForever) {
  collector()->OnAppStartupOrResume();
  collector()->OnSuccessfulNavigationOnline();
  EXPECT_EQ(UsageType::ONLINE, GetAccumulatedUsageFromPrefs());
  collector()->OnSuccessfulNavigationOffline();
  EXPECT_EQ(UsageType::MIXED, GetAccumulatedUsageFromPrefs());
  collector()->OnSuccessfulNavigationOffline();
  collector()->OnSuccessfulNavigationOnline();
  collector()->OnAppStartupOrResume();
  EXPECT_EQ(UsageType::MIXED, GetAccumulatedUsageFromPrefs());
}

// Restore from Prefs keeps accumulated state, counters and timestamp.
TEST_F(OfflineMetricsCollectorTest, RestoreFromPrefs) {
  base::Time start = test_clock()->Now();
  collector()->OnSuccessfulNavigationOnline();
  EXPECT_EQ(UsageType::ONLINE, GetAccumulatedUsageFromPrefs());
  EXPECT_EQ(GetTimestampFromPrefs(), start);

  prefs()->SetInteger(prefs::kOfflineUsageUnusedCount, 1);
  prefs()->SetInteger(prefs::kOfflineUsageStartedCount, 2);
  prefs()->SetInteger(prefs::kOfflineUsageOfflineCount, 3);
  prefs()->SetInteger(prefs::kOfflineUsageOnlineCount, 4);
  prefs()->SetInteger(prefs::kOfflineUsageMixedCount, 5);

  Reload();
  collector()->OnSuccessfulNavigationOffline();
  EXPECT_EQ(UsageType::MIXED, GetAccumulatedUsageFromPrefs());
  EXPECT_EQ(GetTimestampFromPrefs(), start);

  collector()->ReportAccumulatedStats();
  histograms().ExpectBucketCount("OfflinePages.OfflineUsage",
                                 0 /* UsageType::UNUSED */, 1);
  histograms().ExpectBucketCount("OfflinePages.OfflineUsage",
                                 1 /* UsageType::STARTED */, 2);
  histograms().ExpectBucketCount("OfflinePages.OfflineUsage",
                                 2 /* UsageType::OFFLINE */, 3);
  histograms().ExpectBucketCount("OfflinePages.OfflineUsage",
                                 3 /* UsageType::ONLINE */, 4);
  histograms().ExpectBucketCount("OfflinePages.OfflineUsage",
                                 4 /* UsageType::MIXED */, 5);

  // After reporting, counters should be reset.
  EXPECT_EQ(0, prefs()->GetInteger(prefs::kOfflineUsageUnusedCount));
  EXPECT_EQ(0, prefs()->GetInteger(prefs::kOfflineUsageStartedCount));
  EXPECT_EQ(0, prefs()->GetInteger(prefs::kOfflineUsageOfflineCount));
  EXPECT_EQ(0, prefs()->GetInteger(prefs::kOfflineUsageOnlineCount));
  EXPECT_EQ(0, prefs()->GetInteger(prefs::kOfflineUsageMixedCount));
}

TEST_F(OfflineMetricsCollectorTest, ChangesWithinDay) {
  base::Time start = test_clock()->Now();
  collector()->OnAppStartupOrResume();
  collector()->OnSuccessfulNavigationOnline();
  EXPECT_EQ(UsageType::ONLINE, GetAccumulatedUsageFromPrefs());

  // Move time ahead but still same day.
  base::Time later1Hour = start + base::TimeDelta::FromHours(1);
  test_clock()->SetNow(later1Hour);
  collector()->OnSuccessfulNavigationOffline();
  // Timestamp shouldn't change.
  EXPECT_EQ(GetTimestampFromPrefs(), start);
}

TEST_F(OfflineMetricsCollectorTest, MultipleDays) {
  base::Time start = test_clock()->Now();
  collector()->OnAppStartupOrResume();

  base::Time nextDay = start + base::TimeDelta::FromHours(25);
  test_clock()->SetNow(nextDay);

  collector()->OnAppStartupOrResume();
  // 1 day 'started', another is being accumulated...
  EXPECT_EQ(1, prefs()->GetInteger(prefs::kOfflineUsageStartedCount));
  EXPECT_EQ(UsageType::STARTED, GetAccumulatedUsageFromPrefs());

  base::Time skip4Days = nextDay + base::TimeDelta::FromHours(24 * 4);
  test_clock()->SetNow(skip4Days);
  collector()->OnSuccessfulNavigationOnline();
  // 2 days started, 3 days skipped ('unused').
  EXPECT_EQ(2, prefs()->GetInteger(prefs::kOfflineUsageStartedCount));
  EXPECT_EQ(3, prefs()->GetInteger(prefs::kOfflineUsageUnusedCount));

  // No online days in the past..
  EXPECT_EQ(0, prefs()->GetInteger(prefs::kOfflineUsageOnlineCount));
  // .. but current day is 'online' type.
  EXPECT_EQ(UsageType::ONLINE, GetAccumulatedUsageFromPrefs());

  // Other counters not affected.
  EXPECT_EQ(0, prefs()->GetInteger(prefs::kOfflineUsageOfflineCount));
  EXPECT_EQ(0, prefs()->GetInteger(prefs::kOfflineUsageMixedCount));

  // Force collector to report stats and observe them reported correctly.
  collector()->ReportAccumulatedStats();
  histograms().ExpectBucketCount("OfflinePages.OfflineUsage",
                                 0 /* UsageType::UNUSED */, 3);
  histograms().ExpectBucketCount("OfflinePages.OfflineUsage",
                                 1 /* UsageType::STARTED */, 2);
  histograms().ExpectBucketCount("OfflinePages.OfflineUsage",
                                 2 /* UsageType::OFFLINE */, 0);
  histograms().ExpectBucketCount("OfflinePages.OfflineUsage",
                                 3 /* UsageType::ONLINE */, 0);
  histograms().ExpectBucketCount("OfflinePages.OfflineUsage",
                                 4 /* UsageType::MIXED */, 0);
}

}  // namespace offline_pages
