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
  void HistogramBasicTest(
      const std::vector<uint32_t>& first_features,
      const std::vector<uint32_t>& second_features = std::vector<uint32_t>()) {
    NavigateAndCommit(GURL(kTestUrl));
    page_load_metrics::mojom::PageLoadFeatures first_features_;
    first_features_.features = first_features;
    page_load_metrics::mojom::PageLoadFeatures second_features_;
    second_features_.features = second_features;
    SimulateFeaturesUpdate(first_features_);
    for (auto feature : first_features) {
      histogram_tester().ExpectBucketCount(kFeaturesHistogramName, feature, 1);
    }
    SimulateFeaturesUpdate(second_features_);
    for (auto feature : first_features) {
      histogram_tester().ExpectBucketCount(kFeaturesHistogramName, feature, 1);
    }
    for (auto feature : second_features) {
      histogram_tester().ExpectBucketCount(kFeaturesHistogramName, feature, 1);
    }
  }

 protected:
  void RegisterObservers(page_load_metrics::PageLoadTracker* tracker) override {
    tracker->AddObserver(base::MakeUnique<UseCounterPageLoadMetricsObserver>());
  }
};

TEST_F(UseCounterPageLoadMetricsObserverTest, CountOneFeature) {
  HistogramBasicTest(
      std::vector<uint32_t>({static_cast<uint32_t>(WebFeature::kFetch)}));
}

TEST_F(UseCounterPageLoadMetricsObserverTest, CountFeatures) {
  HistogramBasicTest(std::vector<uint32_t>(
      {static_cast<uint32_t>(WebFeature::kFetch),
       static_cast<uint32_t>(WebFeature::kFetch),
       static_cast<uint32_t>(WebFeature::kFetchBodyStream)}));
}

TEST_F(UseCounterPageLoadMetricsObserverTest, CountDuplicatedFeatures) {
  HistogramBasicTest(
      std::vector<uint32_t>(
          {static_cast<uint32_t>(WebFeature::kFetch),
           static_cast<uint32_t>(WebFeature::kFetchBodyStream)}),
      std::vector<uint32_t>(
          {static_cast<uint32_t>(WebFeature::kFetch),
           static_cast<uint32_t>(WebFeature::kSVGSMILAdditiveAnimation)}));
}
