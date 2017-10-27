// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/use_counter_page_load_metrics_observer.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/page_load_metrics/observers/use_counter/ukm_features.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_entry_builder.h"
#include "services/metrics/public/cpp/ukm_recorder.h"

#include "base/metrics/histogram_macros.h"

using WebFeature = blink::mojom::WebFeature;
using Features = page_load_metrics::mojom::PageLoadFeatures;

UseCounterPageLoadMetricsObserver::UseCounterPageLoadMetricsObserver() {}
UseCounterPageLoadMetricsObserver::~UseCounterPageLoadMetricsObserver() {}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
UseCounterPageLoadMetricsObserver::OnCommit(
    content::NavigationHandle* navigation_handle,
    ukm::SourceId source_id) {
  // Verify that no feature usage is observed before commit
  DCHECK(features_recorded_.count() <= 0);
  UMA_HISTOGRAM_ENUMERATION(internal::kFeaturesHistogramName,
                            WebFeature::kPageVisits,
                            WebFeature::kNumberOfFeatures);
  features_recorded_.set(static_cast<size_t>(WebFeature::kPageVisits));
  return CONTINUE_OBSERVING;
}

void UseCounterPageLoadMetricsObserver::OnFeaturesUsageObserved(
    const Features& features,
    const ukm::SourceId source_id) {
  for (auto feature : features.features) {
    // Verify that kPageVisits is only observed at most once per observer.
    DCHECK(feature != WebFeature::kPageVisits);
    // The usage of each feature should be only measured once. With OOPIF,
    // multiple child frames may send the same feature to the browser, skip if
    // feature has already been measured.
    if (features_recorded_.test(static_cast<size_t>(feature)))
      continue;
    UMA_HISTOGRAM_ENUMERATION(internal::kFeaturesHistogramName, feature,
                              WebFeature::kNumberOfFeatures);
    features_recorded_.set(static_cast<size_t>(feature));
    if (IsOptInUKMFeature(feature)) {
      ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
      if (ukm_recorder) {
        std::unique_ptr<ukm::UkmEntryBuilder> builder =
            ukm_recorder->GetEntryBuilder(source_id, "Blink.UseCounter");
        builder->AddMetric("Features", static_cast<int64_t>(feature));
      }
    }
  }
}
