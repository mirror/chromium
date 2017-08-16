// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_STATS_COLLECTOR_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_STATS_COLLECTOR_H_

#include <cstdint>
#include <memory>
#include <string>

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
  enum SessionType {
    kSessionRestore,
    // BackgroundTabOpen session is the duration from the time when the browser
    // starts to open background tabs until the time when browser has finished
    // loading those tabs. During the BackgroundTabOpen session, some background
    // tabs could become foreground due to user action, but that would not
    // affect the session end time point. For example, a browser has tab1 in
    // foreground, and starts to open tab2 and tab3 in background. The session
    // will end after tab2 and tab3 finishes loading, even if tab2 and tab3 are
    // brought to foreground before they are loaded. BackgroundTabOpen session
    // excludes the background tabs being controlled by SessionRestore, so that
    // those two activities are clearly separated.
    kBackgroundTabOpen
  };

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
