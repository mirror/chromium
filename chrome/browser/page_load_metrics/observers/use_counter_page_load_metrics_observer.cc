// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/use_counter_page_load_metrics_observer.h"

#include "base/metrics/histogram_macros.h"

UseCounterPageLoadMetricsObserver::UseCounterPageLoadMetricsObserver()
    : features_recorded_(static_cast<int>(WebFeature::kNumberOfFeatures)) {}

UseCounterPageLoadMetricsObserver::~UseCounterPageLoadMetricsObserver() {}

void UseCounterPageLoadMetricsObserver::OnFeatureUsageObserved(
    const std::vector<WebFeature>& features) {
  for (auto feature : features) {
    RecordMeasurement(static_cast<Sample>(feature));
  }
}

void UseCounterPageLoadMetricsObserver::RecordMeasurement(Sample feature) {
  if (!features_recorded_.test(feature)) {
    UMA_HISTOGRAM_ENUMERATION(
        "Blink.UseCounter.Features_TEST", feature,
        static_cast<uint32_t>(WebFeature::kNumberOfFeatures));
    features_recorded_.set(feature);
  }
}
