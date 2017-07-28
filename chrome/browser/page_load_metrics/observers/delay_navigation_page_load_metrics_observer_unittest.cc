// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/delay_navigation_page_load_metrics_observer.h"

#include <memory>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/time/time.h"
#include "chrome/browser/page_load_metrics/experiments/delay_navigation_throttle.h"
#include "chrome/browser/page_load_metrics/observers/page_load_metrics_observer_test_harness.h"
#include "chrome/browser/page_load_metrics/page_load_tracker.h"
#include "chrome/common/page_load_metrics/test/page_load_metrics_test_util.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/navigation_throttle_inserter.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kDefaultTestUrl[] = "https://example.com/";

const int kDelayMillis = 100;

}  // namespace

class DelayNavigationPageLoadMetricsObserverTest
    : public page_load_metrics::PageLoadMetricsObserverTestHarness {
 public:
  DelayNavigationPageLoadMetricsObserverTest() = default;
  ~DelayNavigationPageLoadMetricsObserverTest() override = default;

  void RegisterObservers(page_load_metrics::PageLoadTracker* tracker) override {
    tracker->AddObserver(
        base::MakeUnique<DelayNavigationPageLoadMetricsObserver>());
  }

  void SetUp() override {
    PageLoadMetricsObserverTestHarness::SetUp();
    mock_time_task_runner_ = new base::TestMockTimeTaskRunner();

    // Create a InjectDelayNavigationThrottleWebContentsObserver, which
    // instantiates a DelayNavigationThrottle for each main frame navigation in
    // the web_contents().
    throttle_inserter_ = base::MakeUnique<content::NavigationThrottleInserter>(
        web_contents(),
        base::BindRepeating(
            &DelayNavigationPageLoadMetricsObserverTest::CreateThrottle,
            base::Unretained(this)));
  }

  std::unique_ptr<content::NavigationThrottle> CreateThrottle(
      content::NavigationHandle* handle) {
    if (!handle->IsInMainFrame())
      return nullptr;
    return base::MakeUnique<DelayNavigationThrottle>(
        handle, mock_time_task_runner_,
        base::TimeDelta::FromMilliseconds(kDelayMillis));
  }

  bool AnyMetricsRecorded() {
    return !histogram_tester()
                .GetTotalCountsForPrefix("PageLoad.Clients.DelayNavigation.")
                .empty();
  }

  void NavigateToDefaultUrlAndCommit() {
    GURL url(kDefaultTestUrl);
    content::RenderFrameHostTester* rfh_tester =
        content::RenderFrameHostTester::For(main_rfh());
    rfh_tester->SimulateNavigationStart(url);

    // There may be other throttles that DEFER and post async tasks to the UI
    // thread. Allow them to run to completion, so our throttle is guaranteed to
    // have a chance to run.
    base::RunLoop().RunUntilIdle();

    EXPECT_TRUE(mock_time_task_runner_->HasPendingTask());
    mock_time_task_runner_->FastForwardBy(
        base::TimeDelta::FromMilliseconds(kDelayMillis));
    EXPECT_FALSE(mock_time_task_runner_->HasPendingTask());

    // Run any remaining async tasks, to make sure all other deferred throttles
    // can complete.
    base::RunLoop().RunUntilIdle();

    rfh_tester->SimulateNavigationCommit(url);
  }

 private:
  std::unique_ptr<content::NavigationThrottleInserter> throttle_inserter_;
  scoped_refptr<base::TestMockTimeTaskRunner> mock_time_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(DelayNavigationPageLoadMetricsObserverTest);
};

TEST_F(DelayNavigationPageLoadMetricsObserverTest, NoMetricsWithoutNavigation) {
  ASSERT_FALSE(AnyMetricsRecorded());
}

TEST_F(DelayNavigationPageLoadMetricsObserverTest, CommitWithPaint) {
  NavigateToDefaultUrlAndCommit();

  page_load_metrics::mojom::PageLoadTiming timing;
  page_load_metrics::InitPageLoadTimingForTest(&timing);
  timing.navigation_start = base::Time::FromDoubleT(1);
  timing.paint_timing->first_paint = base::TimeDelta::FromMilliseconds(1);
  PopulateRequiredTimingFields(&timing);
  SimulateTimingUpdate(timing);

  ASSERT_TRUE(AnyMetricsRecorded());
  histogram_tester().ExpectTotalCount(
      internal::kHistogramNavigationDelaySpecified, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramNavigationDelaySpecified, kDelayMillis, 1);

  histogram_tester().ExpectTotalCount(internal::kHistogramNavigationDelayActual,
                                      1);
  histogram_tester().ExpectTotalCount(internal::kHistogramNavigationDelayDelta,
                                      1);
}
