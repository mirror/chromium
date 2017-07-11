// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/use_counter_page_load_metrics_observer.h"

#include "base/metrics/histogram_macros.h"

typedef blink::mojom::WebFeature WebFeature;
typedef page_load_metrics::mojom::PageLoadFeatures Features;
typedef base::Histogram::Sample Sample;

UseCounterPageLoadMetricsObserver::UseCounterPageLoadMetricsObserver()
    : features_recorded_(kNumberOfFeatures) {}

UseCounterPageLoadMetricsObserver::~UseCounterPageLoadMetricsObserver() {}

void UseCounterPageLoadMetricsObserver::OnFeaturesUsageObserved(
    const Features& features) {
  for (auto feature : features.features) {
    RecordMeasurement(static_cast<base::Histogram::Sample>(feature));
  }
}

void UseCounterPageLoadMetricsObserver::RecordMeasurement(Sample feature) {
  if (!features_recorded_.test(feature)) {
    UMA_HISTOGRAM_ENUMERATION("Blink.UseCounter.Features_TEST", feature,
                              kNumberOfFeatures);
    features_recorded_.set(feature);
  }
}
