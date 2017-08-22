// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_STATS_COLLECTOR_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_STATS_COLLECTOR_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/gtest_prod_util.h"
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
    // SessionRestore is the duration from the time when the browser starts to
    // restore tabs until the time when the browser has finished loading those
    // tabs or when the browser stops loading tabs due to memory pressure.
    // During SessionRestore, some other tabs could be open due to user action,
    // but that would not affect the session end time point. For example, a
    // browser starts to restore tab1 and tab2. Tab3 is open due to user
    // clicking a link in tab1. SessionRestore ends after tab1 and tab2 finishes
    // loading, even if tab3 is still loading.
    kSessionRestore,

    // BackgroundTabOpening session is the duration from the time when the
    // browser starts to open background tabs until the time when browser has
    // finished loading those tabs or paused loading due to memory pressure.
    // A new session starts when the browser resumes loading background tabs
    // when memory pressure returns to normal. During the BackgroundTabOpening
    // session, some background tabs could become foreground due to user action,
    // but that would not affect the session end time point. For example, a
    // browser has tab1 in foreground, and starts to open tab2 and tab3 in
    // background. The session will end after tab2 and tab3 finishes loading,
    // even if tab2 and tab3 are brought to foreground before they are loaded.
    // BackgroundTabOpening session excludes the background tabs being
    // controlled by SessionRestore, so that those two activities are clearly
    // separated.
    kBackgroundTabOpening
  };

  explicit TabManagerStatsCollector(TabManager* tab_manager);
  ~TabManagerStatsCollector();

  // Records UMA histograms for the tab state when switching to a different tab
  // during session restore.
  void RecordSwitchToTab(content::WebContents* contents) const;

  // Record expected task queueing durations of foreground tabs in session
  // restore.
  void RecordExpectedTaskQueueingDuration(content::WebContents* contents,
                                          base::TimeDelta queueing_time);

  // SessionRestoreObserver
  void OnSessionRestoreStartedLoadingTabs() override;
  void OnSessionRestoreFinishedLoadingTabs() override;

  // The following two functions defines the start and end of a background tab
  // opening session.
  void OnBackgroundTabOpeningSessionStarted();
  void OnBackgroundTabOpeningSessionEnded();

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

  bool is_in_background_tab_opening_session() const {
    return is_in_background_tab_opening_session_;
  }

 private:
  class SwapMetricsDelegate;
  FRIEND_TEST_ALL_PREFIXES(TabManagerStatsCollectorTest,
                           HistogramSessionRestoreExpectedTaskQueueingDuration);
  FRIEND_TEST_ALL_PREFIXES(
      TabManagerStatsCollectorTest,
      HistogramBackgroundTabOpeningExpectedTaskQueueingDuration);

  static const char
      kHistogramSessionRestoreForegroundTabExpectedTaskQueueingDuration[];
  static const char
      kHistogramBackgroundTabOpeningForegroundTabExpectedTaskQueueingDuration[];

  TabManager* tab_manager_;
  bool is_session_restore_loading_tabs_;
  bool is_in_background_tab_opening_session_;

  std::unique_ptr<content::SwapMetricsDriver>
      session_restore_swap_metrics_driver_;
  std::unique_ptr<content::SwapMetricsDriver>
      background_tab_opening_swap_metrics_driver_;
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_STATS_COLLECTOR_H_
