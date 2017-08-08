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
// system-related events and properties during session restore.
class TabManagerStatsCollector : public SessionRestoreObserver {
 public:
  enum EventType { kSessionRestore, kBackgroundTabOpen };

  explicit TabManagerStatsCollector(TabManager* tab_manager);
  ~TabManagerStatsCollector();

  // Records UMA histograms for the tab state when switching to a different tab
  // during session restore.
  void RecordSwitchToTab(content::WebContents* contents) const;

  // SessionRestoreObserver
  void OnSessionRestoreStartedLoadingTabs() override;
  void OnSessionRestoreFinishedLoadingTabs() override;

  void OnStartedLoadingBackgroundTabs();
  void OnFinishedLoadingBackgroundTabs();

  // The following record UMA histograms for system swap metrics.
  void OnSwapInCount(EventType type, uint64_t count, base::TimeDelta interval);
  void OnSwapOutCount(EventType type, uint64_t count, base::TimeDelta interval);
  void OnDecompressedPageCount(EventType type,
                               uint64_t count,
                               base::TimeDelta interval);
  void OnCompressedPageCount(EventType type,
                             uint64_t count,
                             base::TimeDelta interval);
  void OnUpdateMetricsFailed(EventType type);

 private:
  class SwapMetricsDelegate;

  std::string GetEventName(EventType type) const;

  TabManager* tab_manager_;
  bool is_session_restore_loading_tabs_;
  std::unique_ptr<content::SwapMetricsDriver>
      session_restore_swap_metrics_driver_;
  std::unique_ptr<content::SwapMetricsDriver>
      background_tab_open_swap_metrics_driver_;
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_STATS_COLLECTOR_H_
