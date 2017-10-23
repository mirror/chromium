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

// Tests that timing of non-preload tabs are collected.
TEST_F(LoadTimingTabHelperTest, ReportMetricForNonPrerenderTab) {
  tab_helper()->DidInitiatePageLoad();
  base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(10));
  tab_helper()->ReportLoadTime();

  histogram_tester_.ExpectTimeBucketCount(
      LoadTimingTabHelper::kOmniboxToPageLoadedMetric,
      base::TimeDelta::FromMilliseconds(10), 1);
}

// Tests that load time of prerender tab is always recorded as 0.
TEST_F(LoadTimingTabHelperTest, ReportMetricForPrerenderTab) {
  tab_helper()->DidInitiatePageLoad();
  tab_helper()->DidPromotePrerenderTab();
  base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(10));
  tab_helper()->ReportLoadTime();

  histogram_tester_.ExpectTimeBucketCount(
      LoadTimingTabHelper::kOmniboxToPageLoadedMetric, base::TimeDelta(), 1);
}

// Tests that load time is not recorded if the timer was not started.
TEST_F(LoadTimingTabHelperTest, MetricNotReportedIfStartTimeNotRecorded) {
  tab_helper()->ReportLoadTime();
  EXPECT_TRUE(histogram_tester_
                  .GetTotalCountsForPrefix(
                      LoadTimingTabHelper::kOmniboxToPageLoadedMetric)
                  .empty());
}

}  // namespace
