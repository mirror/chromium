// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_ANDROID_PAGE_LOAD_METRICS_OBSERVER_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_ANDROID_PAGE_LOAD_METRICS_OBSERVER_H_

#include <jni.h>

#include "base/macros.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"
#include "net/nqe/network_quality_estimator.h"

namespace content {
class WebContents;
}  // namespace content

class GURL;

/** Forwards page load metrics to the Java side on Android. */
class AndroidPageLoadMetricsObserver
    : public page_load_metrics::PageLoadMetricsObserver {
 public:
  explicit AndroidPageLoadMetricsObserver(content::WebContents* web_contents);

  // page_load_metrics::PageLoadMetricsObserver:
  ObservePolicy OnStart(content::NavigationHandle* navigation_handle,
                        const GURL& currently_committed_url,
                        bool started_in_foreground) override;
  void OnFirstContentfulPaintInPage(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info) override;
  void OnLoadEventStart(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& info) override;
  void OnLoadedResource(const page_load_metrics::ExtraRequestCompleteInfo&
                            extra_request_complete_info) override;

 private:
  content::WebContents* web_contents_;

  bool did_dispatch_on_main_resource_ = false;

  net::NetworkQualityEstimator::NetworkQualityProvider*
      network_quality_provider_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(AndroidPageLoadMetricsObserver);
};

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_ANDROID_PAGE_LOAD_METRICS_OBSERVER_H_
