// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_STATS_COLLECTOR_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_STATS_COLLECTOR_H_

#include <cstdint>
#include <memory>

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
// system-related events and properties during session restore or background tab
// opening session.
class TabManagerStatsCollector : public SessionRestoreObserver {
 public:
  enum SessionType { kSessionRestore, kBackgroundTabOpen };

  explicit TabManagerStatsCollector(TabManager* tab_manager);
  ~TabManagerStatsCollector();

  // Records UMA histograms for the tab state when switching to a different tab
  // during session restore.
  void RecordSwitchToTab(content::WebContents* contents) const;

  // SessionRestoreObserver
  void OnSessionRestoreStartedLoadingTabs() override;
  void OnSessionRestoreFinishedLoadingTabs() override;

  // The following two functions defines the start and end of a background tab
  // opening session.
  void OnBackgroundTabOpenSessionStarted();
  void OnBackgroundTabOpenSessionEnded();

  // The following record UMA histograms for system swap metrics.
  void OnSwapInCount(SessionType type,
                     uint64_t count,
                     base::TimeDelta interval);
  void OnSwapOutCount(SessionType type,
                      uint64_t count,
                      base::TimeDelta interval);
  void OnDecompressedPageCount(SessionType type,
                               uint64_t count,
                               base::TimeDelta interval);
  void OnCompressedPageCount(SessionType type,
                             uint64_t count,
                             base::TimeDelta interval);
  void OnUpdateMetricsFailed(SessionType type);

 private:
  class SwapMetricsDelegate;

  std::string GetEventName(SessionType type) const;

  TabManager* tab_manager_;
  bool is_session_restore_loading_tabs_;
  bool is_in_background_tab_open_session_;

  std::unique_ptr<content::SwapMetricsDriver>
      session_restore_swap_metrics_driver_;
  std::unique_ptr<content::SwapMetricsDriver>
      background_tab_open_swap_metrics_driver_;
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_STATS_COLLECTOR_H_
