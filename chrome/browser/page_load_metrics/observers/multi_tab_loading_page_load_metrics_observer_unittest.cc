// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/multi_tab_loading_page_load_metrics_observer.h"

#include "base/memory/ptr_util.h"
#include "base/optional.h"
#include "chrome/browser/page_load_metrics/observers/page_load_metrics_observer_test_harness.h"
#include "chrome/browser/page_load_metrics/page_load_tracker.h"
#include "chrome/common/page_load_metrics/test/page_load_metrics_test_util.h"
#include "content/public/browser/web_contents.h"

namespace {

const char kDefaultTestUrl[] = "https://google.com";

class TestMultiTabLoadingPageLoadMetricsObserver
    : public MultiTabLoadingPageLoadMetricsObserver {
 public:
  explicit TestMultiTabLoadingPageLoadMetricsObserver(
      int number_of_tabs_with_inflight_load)
      : number_of_tabs_with_inflight_load_(number_of_tabs_with_inflight_load) {}
  ~TestMultiTabLoadingPageLoadMetricsObserver() override {}

 private:
  int NumberOfTabsWithInflightLoad(
      content::NavigationHandle* navigation_handle) override {
    return number_of_tabs_with_inflight_load_;
  }

  const int number_of_tabs_with_inflight_load_;
};

}  // namespace

class MultiTabLoadingPageLoadMetricsObserverTest
    : public page_load_metrics::PageLoadMetricsObserverTestHarness {
 public:
  enum TabState { Foreground, Background };

  void SimulatePageLoad(int number_of_tabs_with_inflight_load,
                        TabState tab_state) {
    number_of_tabs_with_inflight_load_ = number_of_tabs_with_inflight_load;

    page_load_metrics::mojom::PageLoadTiming timing;
    page_load_metrics::InitPageLoadTimingForTest(&timing);
    timing.navigation_start = base::Time::FromDoubleT(1);
    timing.paint_timing->first_contentful_paint =
        base::TimeDelta::FromMilliseconds(300);
    timing.paint_timing->first_meaningful_paint =
        base::TimeDelta::FromMilliseconds(700);
    timing.document_timing->dom_content_loaded_event_start =
        base::TimeDelta::FromMilliseconds(600);
    timing.document_timing->load_event_start =
        base::TimeDelta::FromMilliseconds(1000);
    PopulateRequiredTimingFields(&timing);

    if (tab_state == Background) {
      // Start in background.
      web_contents()->WasHidden();
    }

    NavigateAndCommit(GURL(kDefaultTestUrl));

    if (tab_state == Background) {
      // Foreground the tab.
      web_contents()->WasShown();
    }

    SimulateTimingUpdate(timing);
  }

 protected:
  void RegisterObservers(page_load_metrics::PageLoadTracker* tracker) override {
    tracker->AddObserver(
        base::MakeUnique<TestMultiTabLoadingPageLoadMetricsObserver>(
            number_of_tabs_with_inflight_load_.value()));
  }

  void ValidateHistograms(const char* suffix,
                          base::HistogramBase::Count expected_base,
                          base::HistogramBase::Count expected_2_or_more,
                          base::HistogramBase::Count expected_5_or_more) {
    histogram_tester().ExpectTotalCount(
        std::string(internal::kHistogramPrefixMultiTabLoading).append(suffix),
        expected_base);
    histogram_tester().ExpectTotalCount(
        std::string(internal::kHistogramPrefixMultiTabLoading2OrMore)
            .append(suffix),
        expected_2_or_more);
    histogram_tester().ExpectTotalCount(
        std::string(internal::kHistogramPrefixMultiTabLoading5OrMore)
            .append(suffix),
        expected_5_or_more);
  }

 private:
  base::Optional<int> number_of_tabs_with_inflight_load_;
};

TEST_F(MultiTabLoadingPageLoadMetricsObserverTest, SingleTabLoading) {
  SimulatePageLoad(0, Foreground);
  histogram_tester().ExpectUniqueSample(
      internal::kHistogramMultiTabLoadingNumTabsWithInflightLoad, 0, 1);

  ValidateHistograms(internal::kHistogramSuffixFirstContentfulPaint, 0, 0, 0);
  ValidateHistograms(internal::kHistogramSuffixForegroundToFirstContentfulPaint,
                     0, 0, 0);
  ValidateHistograms(internal::kHistogramSuffixFirstMeaningfulPaint, 0, 0, 0);
  ValidateHistograms(internal::kHistogramSuffixForegroundToFirstMeaningfulPaint,
                     0, 0, 0);
  ValidateHistograms(internal::kHistogramSuffixDomContentLoaded, 0, 0, 0);
  ValidateHistograms(internal::kBackgroundHistogramSuffixDomContentLoaded, 0, 0,
                     0);
  ValidateHistograms(internal::kHistogramSuffixLoad, 0, 0, 0);
  ValidateHistograms(internal::kBackgroundHistogramSuffixLoad, 0, 0, 0);
}

TEST_F(MultiTabLoadingPageLoadMetricsObserverTest, MultiTabLoading1) {
  SimulatePageLoad(1, Foreground);
  histogram_tester().ExpectUniqueSample(
      internal::kHistogramMultiTabLoadingNumTabsWithInflightLoad, 1, 1);

  ValidateHistograms(internal::kHistogramSuffixFirstContentfulPaint, 1, 0, 0);
  ValidateHistograms(internal::kHistogramSuffixForegroundToFirstContentfulPaint,
                     0, 0, 0);
  ValidateHistograms(internal::kHistogramSuffixFirstMeaningfulPaint, 1, 0, 0);
  ValidateHistograms(internal::kHistogramSuffixForegroundToFirstMeaningfulPaint,
                     0, 0, 0);
  ValidateHistograms(internal::kHistogramSuffixDomContentLoaded, 1, 0, 0);
  ValidateHistograms(internal::kBackgroundHistogramSuffixDomContentLoaded, 0, 0,
                     0);
  ValidateHistograms(internal::kHistogramSuffixLoad, 1, 0, 0);
  ValidateHistograms(internal::kBackgroundHistogramSuffixLoad, 0, 0, 0);
}

TEST_F(MultiTabLoadingPageLoadMetricsObserverTest, MultiTabLoading2) {
  SimulatePageLoad(2, Foreground);
  histogram_tester().ExpectUniqueSample(
      internal::kHistogramMultiTabLoadingNumTabsWithInflightLoad, 2, 1);

  ValidateHistograms(internal::kHistogramSuffixFirstContentfulPaint, 1, 1, 0);
  ValidateHistograms(internal::kHistogramSuffixForegroundToFirstContentfulPaint,
                     0, 0, 0);
  ValidateHistograms(internal::kHistogramSuffixFirstMeaningfulPaint, 1, 1, 0);
  ValidateHistograms(internal::kHistogramSuffixForegroundToFirstMeaningfulPaint,
                     0, 0, 0);
  ValidateHistograms(internal::kHistogramSuffixDomContentLoaded, 1, 1, 0);
  ValidateHistograms(internal::kBackgroundHistogramSuffixDomContentLoaded, 0, 0,
                     0);
  ValidateHistograms(internal::kHistogramSuffixLoad, 1, 1, 0);
  ValidateHistograms(internal::kBackgroundHistogramSuffixLoad, 0, 0, 0);
}

TEST_F(MultiTabLoadingPageLoadMetricsObserverTest, MultiTabLoading5) {
  SimulatePageLoad(5, Foreground);
  histogram_tester().ExpectUniqueSample(
      internal::kHistogramMultiTabLoadingNumTabsWithInflightLoad, 5, 1);

  ValidateHistograms(internal::kHistogramSuffixFirstContentfulPaint, 1, 1, 1);
  ValidateHistograms(internal::kHistogramSuffixForegroundToFirstContentfulPaint,
                     0, 0, 0);
  ValidateHistograms(internal::kHistogramSuffixFirstMeaningfulPaint, 1, 1, 1);
  ValidateHistograms(internal::kHistogramSuffixForegroundToFirstMeaningfulPaint,
                     0, 0, 0);
  ValidateHistograms(internal::kHistogramSuffixDomContentLoaded, 1, 1, 1);
  ValidateHistograms(internal::kBackgroundHistogramSuffixDomContentLoaded, 0, 0,
                     0);
  ValidateHistograms(internal::kHistogramSuffixLoad, 1, 1, 1);
  ValidateHistograms(internal::kBackgroundHistogramSuffixLoad, 0, 0, 0);
}

TEST_F(MultiTabLoadingPageLoadMetricsObserverTest, MultiTabBackground) {
  SimulatePageLoad(1, Background);
  histogram_tester().ExpectUniqueSample(
      internal::kHistogramMultiTabLoadingNumTabsWithInflightLoad, 1, 1);

  ValidateHistograms(internal::kHistogramSuffixFirstContentfulPaint, 0, 0, 0);
  ValidateHistograms(internal::kHistogramSuffixForegroundToFirstContentfulPaint,
                     1, 0, 0);
  ValidateHistograms(internal::kHistogramSuffixFirstMeaningfulPaint, 0, 0, 0);
  ValidateHistograms(internal::kHistogramSuffixForegroundToFirstMeaningfulPaint,
                     1, 0, 0);
  ValidateHistograms(internal::kHistogramSuffixDomContentLoaded, 0, 0, 0);
  ValidateHistograms(internal::kBackgroundHistogramSuffixDomContentLoaded, 1, 0,
                     0);
  ValidateHistograms(internal::kHistogramSuffixLoad, 0, 0, 0);
  ValidateHistograms(internal::kBackgroundHistogramSuffixLoad, 1, 0, 0);
}
