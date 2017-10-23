// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/load_timing_tab_helper.h"

#include <memory>
#include <vector>

#include "base/test/histogram_tester.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace {

class LoadTimingTabHelperTest : public PlatformTest {
 protected:
  LoadTimingTabHelperTest() {
    LoadTimingTabHelper::CreateForWebState(&web_state_);
  }

  LoadTimingTabHelper* tab_helper() {
    return LoadTimingTabHelper::FromWebState(&web_state_);
  }

  web::TestWebState web_state_;
  base::HistogramTester histogram_tester_;
};

// Tests that the metric is recorded in a normal page load.
TEST_F(LoadTimingTabHelperTest, ReportMetric) {
  tab_helper()->DidInitiatePageLoad();
  tab_helper()->ResetAndReportLoadTime();

  histogram_tester_.ExpectTimeBucketCount(
      LoadTimingTabHelper::kOmniboxToPageLoadedMetric, base::TimeDelta(), 1);

  // Timer should be reset now and no new sample should be reported without
  // DidInitiatePageLoad().
  tab_helper()->ResetAndReportLoadTime();

  base::HistogramTester::CountsMap expected_counts;
  expected_counts[LoadTimingTabHelper::kOmniboxToPageLoadedMetric] = 1;
  EXPECT_THAT(histogram_tester_.GetTotalCountsForPrefix(
                  LoadTimingTabHelper::kOmniboxToPageLoadedMetric),
              testing::ContainerEq(expected_counts));
}

// Tests that load time is not recorded if the timer was not started.
TEST_F(LoadTimingTabHelperTest, MetricNotReportedIfStartTimeNotRecorded) {
  tab_helper()->ResetAndReportLoadTime();
  EXPECT_TRUE(histogram_tester_
                  .GetTotalCountsForPrefix(
                      LoadTimingTabHelper::kOmniboxToPageLoadedMetric)
                  .empty());
}

// Tests that Reset() after DidInitiatePageLoad() clears the counter.
TEST_F(LoadTimingTabHelperTest, MetricNotReportedIfReset) {
  tab_helper()->DidInitiatePageLoad();
  tab_helper()->Reset();
  tab_helper()->ResetAndReportLoadTime();

  EXPECT_TRUE(histogram_tester_
                  .GetTotalCountsForPrefix(
                      LoadTimingTabHelper::kOmniboxToPageLoadedMetric)
                  .empty());
}

}  // namespace
