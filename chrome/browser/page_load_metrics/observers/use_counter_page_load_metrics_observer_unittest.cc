// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/use_counter_page_load_metrics_observer.h"

#include "base/memory/ptr_util.h"
#include "base/test/histogram_tester.h"
#include "chrome/browser/page_load_metrics/observers/page_load_metrics_observer_test_harness.h"

namespace {
const char kFeaturesHistogramName[] = "Blink.UseCounter.Features_TEST";
const char kTestUrl[] = "https://www.google.com";
typedef blink::WebFeature WebFeature;
}  // namespace

class UseCounterPageLoadMetricsObserverTest
    : public page_load_metrics::PageLoadMetricsObserverTestHarness {
 public:
  void HistogramBasicTest(const std::vector<WebFeature>& first_features,
                          const std::vector<WebFeature>& second_features =
                              std::vector<WebFeature>()) {
    NavigateAndCommit(GURL(kTestUrl));
    SimulateFeaturesUpdate(first_features);
    for (WebFeature feature : first_features) {
      histogram_tester().ExpectBucketCount(
          kFeaturesHistogramName,
          static_cast<base::HistogramBase::Sample>(feature), 1);
    }
    SimulateFeaturesUpdate(second_features);
    for (WebFeature feature : first_features) {
      histogram_tester().ExpectBucketCount(
          kFeaturesHistogramName,
          static_cast<base::HistogramBase::Sample>(feature), 1);
    }
    for (WebFeature feature : second_features) {
      histogram_tester().ExpectBucketCount(
          kFeaturesHistogramName,
          static_cast<base::HistogramBase::Sample>(feature), 1);
    }
  }

 protected:
  void RegisterObservers(page_load_metrics::PageLoadTracker* tracker) override {
    tracker->AddObserver(base::MakeUnique<UseCounterPageLoadMetricsObserver>());
  }
};

TEST_F(UseCounterPageLoadMetricsObserverTest, CountOneFeature) {
  HistogramBasicTest(std::vector<WebFeature>({WebFeature::kFetch}));
}

TEST_F(UseCounterPageLoadMetricsObserverTest, CountFeatures) {
  HistogramBasicTest(std::vector<WebFeature>(
      {WebFeature::kFetch, WebFeature::kFetch, WebFeature::kFetchBodyStream}));
}

TEST_F(UseCounterPageLoadMetricsObserverTest, CountDuplicatedFeatures) {
  HistogramBasicTest(
      std::vector<WebFeature>(
          {WebFeature::kFetch, WebFeature::kFetchBodyStream}),
      std::vector<WebFeature>(
          {WebFeature::kFetch, WebFeature::kSVGSMILAdditiveAnimation}));
}
