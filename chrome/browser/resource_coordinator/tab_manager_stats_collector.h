// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_STATS_COLLECTOR_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_STATS_COLLECTOR_H_

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "base/time/time.h"
#include "chrome/browser/sessions/session_restore_observer.h"

namespace base {
class TimeDelta;
}

namespace content {
class SwapMetricsDriver;
class WebContents;
}  // namespace content

namespace resource_coordinator {

class TabManager;

// TabManagerStatsCollector records UMAs on behalf of TabManager for tab and
// system-related events and properties during session restore.
class TabManagerStatsCollector : public SessionRestoreObserver {
 public:
  explicit TabManagerStatsCollector(TabManager* tab_manager);
  ~TabManagerStatsCollector();

  // Records UMA histograms for the tab state when switching to a different tab
  // during session restore.
  void RecordSwitchToTab(content::WebContents* old_contents,
                         content::WebContents* new_contents);

  // SessionRestoreObserver
  void OnSessionRestoreStartedLoadingTabs() override;
  void OnSessionRestoreFinishedLoadingTabs() override;

  // The following record UMA histograms for system swap metrics during session
  // restore.
  void OnSessionRestoreSwapInCount(uint64_t count, base::TimeDelta interval);
  void OnSessionRestoreSwapOutCount(uint64_t count, base::TimeDelta interval);
  void OnSessionRestoreDecompressedPageCount(uint64_t count,
                                             base::TimeDelta interval);
  void OnSessionRestoreCompressedPageCount(uint64_t count,
                                           base::TimeDelta interval);
  void OnSessionRestoreUpdateMetricsFailed();

  // Called by TabManager when a tab finishes loading. Used as the signal to
  // record tab switch load time metrics for |contents|.
  void OnDidStopLoading(content::WebContents* contents);

  // Called by TabManager when a WebContents is destroyed. Used to clean up
  // |foreground_contents_switched_to_times_| if we were tracking this tab and
  // OnDidStopLoading has not yet been called for it.
  void OnWebContentsDestroyed(content::WebContents* contents);

 private:
  class SessionRestoreSwapMetricsDelegate;

  TabManager* tab_manager_;
  std::unique_ptr<content::SwapMetricsDriver>
      session_restore_swap_metrics_driver_;
  bool is_session_restore_loading_tabs_;
  // The set of foreground tabs that are not yet loaded, mapped to the time that
  // they were switched to. The values represent the start times used for tab
  // switch load time metrics, and are only used during session restore and
  // background tab loading.
  std::unordered_map<content::WebContents*, base::TimeTicks>
      foreground_contents_switched_to_times_;
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_STATS_COLLECTOR_H_
