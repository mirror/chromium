// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/use_counter_page_load_metrics_observer.h"

#include "base/metrics/histogram_macros.h"

using WebFeature = blink::mojom::WebFeature;
using Features = page_load_metrics::mojom::PageLoadFeatures;

UseCounterPageLoadMetricsObserver::UseCounterPageLoadMetricsObserver()
    : features_recorded_(static_cast<int>(WebFeature::kNumberOfFeatures)) {}

UseCounterPageLoadMetricsObserver::~UseCounterPageLoadMetricsObserver() {}

void UseCounterPageLoadMetricsObserver::OnFeaturesUsageObserved(
    const Features& features) {
  for (auto feature : features.features) {
    RecordMeasurement(feature);
  }
}

void UseCounterPageLoadMetricsObserver::RecordMeasurement(WebFeature feature) {
  if (!features_recorded_.test(static_cast<int>(feature))) {
    UMA_HISTOGRAM_ENUMERATION("Blink.UseCounter.Features_TEST", feature,
                              WebFeature::kNumberOfFeatures);
    features_recorded_.set(static_cast<int>(feature));
  }
}
