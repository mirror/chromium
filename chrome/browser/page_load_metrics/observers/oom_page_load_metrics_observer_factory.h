// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_OOM_PAGE_LOAD_METRICS_OBSERVER_FACTORY_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_OOM_PAGE_LOAD_METRICS_OBSERVER_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "chrome/browser/metrics/oom/out_of_memory_reporter.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"
#include "components/ukm/ukm_source.h"
#include "content/public/browser/web_contents_user_data.h"

// This class is a WebContentsUserData and is responsible for creating page load
// metrics observers that are used to associated with OOMs. Note that this class
// is responsible for metrics logging (and not the actual observer), because OOM
// data will often come after a page load metrics observer is torn down.
//
// The class wants to hook into page_load_metrics to make sure counts are
// comparable to page load counts (e.g. we use the same page filtering logic).
class OomPageLoadMetricsObserverFactory
    : public content::WebContentsUserData<OomPageLoadMetricsObserverFactory>,
      public OutOfMemoryReporter::Observer {
 public:
  ~OomPageLoadMetricsObserverFactory() override;

  std::unique_ptr<page_load_metrics::PageLoadMetricsObserver> CreateObserver();

  void OnCommit(content::NavigationHandle* handle, ukm::SourceId source_id);

 private:
  friend class content::WebContentsUserData<OomPageLoadMetricsObserverFactory>;

  explicit OomPageLoadMetricsObserverFactory(
      content::WebContents* web_contents);

  // OutOfMemoryReporter::Observer:
  void OnForegroundOOMDetected(const GURL& url,
                               ukm::SourceId source_id) override;

  ScopedObserver<OutOfMemoryReporter, OutOfMemoryReporter::Observer>
      oom_observer_;

  ukm::SourceId last_committed_source_id_ = ukm::kInvalidSourceId;

  // Count the number of OOM reloads in a row.
  int num_oom_reloads_ = 0;

  bool last_committed_page_did_oom_ = false;
  bool last_committed_page_is_reload_ = false;

  bool previous_committed_page_did_oom_ = false;

  DISALLOW_COPY_AND_ASSIGN(OomPageLoadMetricsObserverFactory);
};

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_OOM_PAGE_LOAD_METRICS_OBSERVER_FACTORY_H_
