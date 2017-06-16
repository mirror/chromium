// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_USE_COUNTER_PAGE_LOAD_METRICS_OBSERVER_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_USE_COUNTER_PAGE_LOAD_METRICS_OBSERVER_H_

#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"

#include <bitset>
#include "base/metrics/histogram.h"

typedef blink::WebFeature WebFeature;
typedef base::Histogram::Sample Sample;

class UseCounterPageLoadMetricsObserver
    : public page_load_metrics::PageLoadMetricsObserver {
 public:
  UseCounterPageLoadMetricsObserver();
  ~UseCounterPageLoadMetricsObserver() override;

  void OnFeatureUsageObserved(const std::vector<WebFeature>&) override;

 private:
  static const int kNumberOfFeatures =
      static_cast<int>(WebFeature::kNumberOfFeatures);
  void RecordMeasurement(Sample);

  std::bitset<kNumberOfFeatures> features_recorded_;
};

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_USE_COUNTER_PAGE_LOAD_METRICS_OBSERVER_H_
