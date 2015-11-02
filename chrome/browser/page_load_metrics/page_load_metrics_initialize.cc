// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/page_load_metrics_initialize.h"

#include "chrome/browser/page_load_metrics/observers/from_gws_page_load_metrics_observer.h"
#include "components/page_load_metrics/browser/metrics_web_contents_observer.h"
#include "components/rappor/rappor_service.h"
#include "content/public/browser/web_contents.h"

namespace {

void RegisterPageLoadMetricsObservers(
    page_load_metrics::PageLoadMetricsObservable* metrics) {
  // Attach observers scoped to the web contents here.

  // This is a self-destruct class, and will delete itself when triggered by
  // OnPageLoadMetricsGoingAway.
  metrics->AddObserver(new FromGWSPageLoadMetricsObserver(metrics));
}

}  // namespace

namespace chrome {

void InitializePageLoadMetricsForWebContents(
    content::WebContents* web_contents,
    rappor::RapporService* rappor_service) {
  RegisterPageLoadMetricsObservers(
      page_load_metrics::MetricsWebContentsObserver::CreateForWebContents(
          web_contents, rappor_service));
}

}  // namespace chrome
