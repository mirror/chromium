// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_USE_COUNTER_PAGE_LOAD_METRICS_OBSERVER_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_USE_COUNTER_PAGE_LOAD_METRICS_OBSERVER_H_

#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"

#include <bitset>
#include "base/metrics/histogram.h"

class UseCounterPageLoadMetricsObserver
    : public page_load_metrics::PageLoadMetricsObserver {
 public:
  UseCounterPageLoadMetricsObserver();
  ~UseCounterPageLoadMetricsObserver() override;

  void OnFeaturesUsageObserved(
      const page_load_metrics::mojom::PageLoadFeatures&) override;

 private:
  void RecordMeasurement(const blink::mojom::WebFeature);

  std::bitset<static_cast<int>(blink::mojom::WebFeature::kNumberOfFeatures)>
      features_recorded_;
};

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_USE_COUNTER_PAGE_LOAD_METRICS_OBSERVER_H_
