// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/session_restore_page_load_metrics_observer.h"

#include <base/debug/stack_trace.h>
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_util.h"
#include "chrome/browser/resource_coordinator/tab_manager.h"
#include "chrome/browser/sessions/session_restore.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace internal {

const char kHistogramSessionRestoreForegroundTabFirstPaint[] =
    "TabManager.Experimental.SessionRestore.ForegroundTab.FirstPaint";
const char kHistogramSessionRestoreForegroundTabFirstContentfulPaint[] =
    "TabManager.Experimental.SessionRestore.ForegroundTab.FirstContentfulPaint";
const char kHistogramSessionRestoreForegroundTabFirstMeaningfulPaint[] =
    "TabManager.Experimental.SessionRestore.ForegroundTab.FirstMeaningfulPaint";

}  // namespace internal

SessionRestorePageLoadMetricsObserver::SessionRestorePageLoadMetricsObserver() {
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
SessionRestorePageLoadMetricsObserver::OnStart(
    content::NavigationHandle* navigation_handle,
    const GURL& currently_committed_url,
    bool started_in_foreground) {
  content::WebContents* contents = navigation_handle->GetWebContents();
  resource_coordinator::TabManager* tab_manager =
      g_browser_process->GetTabManager();
  // Should not be null because this is used only on supported platforms.
  DCHECK(tab_manager);

  return (tab_manager->IsTabInSessionRestore(contents) &&
          tab_manager->IsTabRestoredInForeground(contents) &&
          started_in_foreground)
             ? CONTINUE_OBSERVING
             : STOP_OBSERVING;
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
SessionRestorePageLoadMetricsObserver::OnHidden(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  return STOP_OBSERVING;
}

void SessionRestorePageLoadMetricsObserver::OnFirstPaintInPage(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& extra_info) {
  PAGE_LOAD_HISTOGRAM(internal::kHistogramSessionRestoreForegroundTabFirstPaint,
                      timing.paint_timing->first_paint.value());
}

void SessionRestorePageLoadMetricsObserver::OnFirstContentfulPaintInPage(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& extra_info) {
  PAGE_LOAD_HISTOGRAM(
      internal::kHistogramSessionRestoreForegroundTabFirstContentfulPaint,
      timing.paint_timing->first_contentful_paint.value());
}

void SessionRestorePageLoadMetricsObserver::
    OnFirstMeaningfulPaintInMainFrameDocument(
        const page_load_metrics::mojom::PageLoadTiming& timing,
        const page_load_metrics::PageLoadExtraInfo& extra_info) {
  PAGE_LOAD_HISTOGRAM(
      internal::kHistogramSessionRestoreForegroundTabFirstMeaningfulPaint,
      timing.paint_timing->first_meaningful_paint.value());
}
