// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_manager_stats_collector.h"

#include <cstdint>
#include <memory>
#include <unordered_set>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "base/time/time.h"
#include "chrome/browser/resource_coordinator/tab_manager_web_contents_data.h"
#include "chrome/browser/sessions/session_restore.h"
#include "content/public/browser/swap_metrics_driver.h"

namespace resource_coordinator {

void TabManagerStatsCollector::BackgroundTabStats::Reset() {
  tab_count = 0u;
  tabs_paused = 0u;
  tabs_load_auto_started = 0u;
  tabs_load_user_initiated = 0u;
}

class TabManagerStatsCollector::SwapMetricsDelegate
    : public content::SwapMetricsDriver::Delegate {
 public:
  explicit SwapMetricsDelegate(
      TabManagerStatsCollector* tab_manager_stats_collector,
      SessionType type)
      : tab_manager_stats_collector_(tab_manager_stats_collector),
        event_type_(type) {}

  ~SwapMetricsDelegate() override = default;

  void OnSwapInCount(uint64_t count, base::TimeDelta interval) override {
    tab_manager_stats_collector_->OnSwapInCount(event_type_, count, interval);
  }

  void OnSwapOutCount(uint64_t count, base::TimeDelta interval) override {
    tab_manager_stats_collector_->OnSwapOutCount(event_type_, count, interval);
  }

  void OnDecompressedPageCount(uint64_t count,
                               base::TimeDelta interval) override {
    tab_manager_stats_collector_->OnDecompressedPageCount(event_type_, count,
                                                          interval);
  }

  void OnCompressedPageCount(uint64_t count,
                             base::TimeDelta interval) override {
    tab_manager_stats_collector_->OnCompressedPageCount(event_type_, count,
                                                        interval);
  }

  void OnUpdateMetricsFailed() override {
    tab_manager_stats_collector_->OnUpdateMetricsFailed(event_type_);
  }

 private:
  TabManagerStatsCollector* tab_manager_stats_collector_;
  const SessionType event_type_;
};

TabManagerStatsCollector::TabManagerStatsCollector()
    : is_session_restore_loading_tabs_(false),
      is_in_background_tab_opening_session_(false) {
  session_restore_swap_metrics_driver_ = content::SwapMetricsDriver::Create(
      base::WrapUnique<content::SwapMetricsDriver::Delegate>(
          new SwapMetricsDelegate(this, kSessionRestore)),
      base::TimeDelta::FromSeconds(0));
  background_tab_opening_swap_metrics_driver_ =
      content::SwapMetricsDriver::Create(
          base::WrapUnique<content::SwapMetricsDriver::Delegate>(
              new SwapMetricsDelegate(this, kBackgroundTabOpening)),
          base::TimeDelta::FromSeconds(0));

  SessionRestore::AddObserver(this);
}

TabManagerStatsCollector::~TabManagerStatsCollector() {
  SessionRestore::RemoveObserver(this);
}

void TabManagerStatsCollector::RecordSwitchToTab(
    content::WebContents* old_contents,
    content::WebContents* new_contents) {
  if (!is_session_restore_loading_tabs_ &&
      !is_in_background_tab_opening_session_) {
    return;
  }

  auto* new_data = TabManager::WebContentsData::FromWebContents(new_contents);
  DCHECK(new_data);

  if (is_session_restore_loading_tabs_) {
    UMA_HISTOGRAM_ENUMERATION(kHistogramSessionRestoreSwitchToTab,
                              new_data->tab_loading_state(),
                              TAB_LOADING_STATE_MAX);
  }
  if (is_in_background_tab_opening_session_) {
    UMA_HISTOGRAM_ENUMERATION(kHistogramBackgroundTabOpeningSwitchToTab,
                              new_data->tab_loading_state(),
                              TAB_LOADING_STATE_MAX);
  }
  if (old_contents)
    foreground_contents_switched_to_times_.erase(old_contents);
  DCHECK(
      !base::ContainsKey(foreground_contents_switched_to_times_, new_contents));
  if (new_data->tab_loading_state() != TAB_IS_LOADED) {
    foreground_contents_switched_to_times_.insert(
        std::make_pair(new_contents, base::TimeTicks::Now()));
  }
}

void TabManagerStatsCollector::RecordExpectedTaskQueueingDuration(
    content::WebContents* contents,
    base::TimeDelta queueing_time) {
  if (!contents->IsVisible())
    return;

  if (is_session_restore_loading_tabs_) {
    UMA_HISTOGRAM_TIMES(
        kHistogramSessionRestoreForegroundTabExpectedTaskQueueingDuration,
        queueing_time);
  }

  if (is_in_background_tab_opening_session_) {
    UMA_HISTOGRAM_TIMES(
        kHistogramBackgroundTabOpeningForegroundTabExpectedTaskQueueingDuration,
        queueing_time);
  }
}

void TabManagerStatsCollector::OnSessionRestoreStartedLoadingTabs() {
  DCHECK(!is_session_restore_loading_tabs_);
  if (session_restore_swap_metrics_driver_)
    session_restore_swap_metrics_driver_->InitializeMetrics();
  is_session_restore_loading_tabs_ = true;
}

void TabManagerStatsCollector::OnSessionRestoreFinishedLoadingTabs() {
  DCHECK(is_session_restore_loading_tabs_);
  if (session_restore_swap_metrics_driver_)
    session_restore_swap_metrics_driver_->UpdateMetrics();
  is_session_restore_loading_tabs_ = false;
}

void TabManagerStatsCollector::OnBackgroundTabOpeningSessionStarted() {
  DCHECK(!is_in_background_tab_opening_session_);
  background_tab_stats_.Reset();
  if (background_tab_opening_swap_metrics_driver_)
    background_tab_opening_swap_metrics_driver_->InitializeMetrics();
  is_in_background_tab_opening_session_ = true;
}

void TabManagerStatsCollector::OnBackgroundTabOpeningSessionEnded() {
  DCHECK(is_in_background_tab_opening_session_);
  UMA_HISTOGRAM_COUNTS_100("TabManager.BackgroundTabOpening.TabCount",
                           background_tab_stats_.tab_count);
  UMA_HISTOGRAM_COUNTS_100("TabManager.BackgroundTabOpening.TabsPaused",
                           background_tab_stats_.tabs_paused);
  UMA_HISTOGRAM_COUNTS_100(
      "TabManager.BackgroundTabOpening.TabsLoadAutoStarted",
      background_tab_stats_.tabs_load_auto_started);
  UMA_HISTOGRAM_COUNTS_100(
      "TabManager.BackgroundTabOpening.TabsLoadUserInitiated",
      background_tab_stats_.tabs_load_user_initiated);

  if (background_tab_opening_swap_metrics_driver_)
    background_tab_opening_swap_metrics_driver_->UpdateMetrics();
  is_in_background_tab_opening_session_ = false;
}

void TabManagerStatsCollector::OnSwapInCount(SessionType type,
                                             uint64_t count,
                                             base::TimeDelta interval) {
  switch (type) {
    case kSessionRestore:
      DCHECK(is_session_restore_loading_tabs_);
      UMA_HISTOGRAM_COUNTS_10000(
          "TabManager.SessionRestore.SwapInPerSecond",
          static_cast<double>(count) / interval.InSecondsF());
      break;
    case kBackgroundTabOpening:
      DCHECK(is_in_background_tab_opening_session_);
      UMA_HISTOGRAM_COUNTS_10000(
          "TabManager.BackgroundTabOpening.SwapInPerSecond",
          static_cast<double>(count) / interval.InSecondsF());
      break;
    default:
      NOTREACHED();
  }
}

void TabManagerStatsCollector::OnSwapOutCount(SessionType type,
                                              uint64_t count,
                                              base::TimeDelta interval) {
  switch (type) {
    case kSessionRestore:
      DCHECK(is_session_restore_loading_tabs_);
      UMA_HISTOGRAM_COUNTS_10000(
          "TabManager.SessionRestore.SwapOutPerSecond",
          static_cast<double>(count) / interval.InSecondsF());
      break;
    case kBackgroundTabOpening:
      DCHECK(is_in_background_tab_opening_session_);
      UMA_HISTOGRAM_COUNTS_10000(
          "TabManager.BackgroundTabOpening.SwapOutPerSecond",
          static_cast<double>(count) / interval.InSecondsF());
      break;
    default:
      NOTREACHED();
  }
}

void TabManagerStatsCollector::OnDecompressedPageCount(
    SessionType type,
    uint64_t count,
    base::TimeDelta interval) {
  switch (type) {
    case kSessionRestore:
      DCHECK(is_session_restore_loading_tabs_);
      UMA_HISTOGRAM_COUNTS_10000(
          "TabManager.SessionRestore.DecompressedPagesPerSecond",
          static_cast<double>(count) / interval.InSecondsF());
      break;
    case kBackgroundTabOpening:
      DCHECK(is_in_background_tab_opening_session_);
      UMA_HISTOGRAM_COUNTS_10000(
          "TabManager.BackgroundTabOpening.DecompressedPagesPerSecond",
          static_cast<double>(count) / interval.InSecondsF());
      break;
    default:
      NOTREACHED();
  }
}

void TabManagerStatsCollector::OnCompressedPageCount(SessionType type,
                                                     uint64_t count,
                                                     base::TimeDelta interval) {
  switch (type) {
    case kSessionRestore:
      DCHECK(is_session_restore_loading_tabs_);
      UMA_HISTOGRAM_COUNTS_10000(
          "TabManager.SessionRestore.CompressedPagesPerSecond",
          static_cast<double>(count) / interval.InSecondsF());
      break;
    case kBackgroundTabOpening:
      DCHECK(is_in_background_tab_opening_session_);
      UMA_HISTOGRAM_COUNTS_10000(
          "TabManager.BackgroundTabOpening.CompressedPagesPerSecond",
          static_cast<double>(count) / interval.InSecondsF());
      break;
    default:
      NOTREACHED();
  }
}

void TabManagerStatsCollector::OnUpdateMetricsFailed(SessionType type) {
  // If either InitializeMetrics() or UpdateMetrics() fails, it's unlikely an
  // error that can be recovered from, in which case we don't collect swap
  // metrics for session restore.
  switch (type) {
    case kSessionRestore:
      session_restore_swap_metrics_driver_.reset();
      break;
    case kBackgroundTabOpening:
      background_tab_opening_swap_metrics_driver_.reset();
      break;
    default:
      NOTREACHED();
  }
}

void TabManagerStatsCollector::OnDidStopLoading(
    content::WebContents* contents) {
  if (!base::ContainsKey(foreground_contents_switched_to_times_, contents))
    return;
  if (is_session_restore_loading_tabs_) {
    UMA_HISTOGRAM_TIMES(kHistogramSessionRestoreTabSwitchLoadTime,
                        base::TimeTicks::Now() -
                            foreground_contents_switched_to_times_[contents]);
  }
  if (is_in_background_tab_opening_session_) {
    UMA_HISTOGRAM_TIMES(kHistogramBackgroundTabOpeningTabSwitchLoadTime,
                        base::TimeTicks::Now() -
                            foreground_contents_switched_to_times_[contents]);
  }
  foreground_contents_switched_to_times_.erase(contents);
}

void TabManagerStatsCollector::OnWebContentsDestroyed(
    content::WebContents* contents) {
  foreground_contents_switched_to_times_.erase(contents);
}

// static
const char TabManagerStatsCollector::
    kHistogramSessionRestoreForegroundTabExpectedTaskQueueingDuration[] =
        "TabManager.SessionRestore.ForegroundTab.ExpectedTaskQueueingDuration";

// static
const char TabManagerStatsCollector::
    kHistogramBackgroundTabOpeningForegroundTabExpectedTaskQueueingDuration[] =
        "TabManager.BackgroundTabOpening.ForegroundTab."
        "ExpectedTaskQueueingDuration";

// Static
const char TabManagerStatsCollector::kHistogramSessionRestoreSwitchToTab[] =
    "TabManager.SessionRestore.SwitchToTab";

// Static
const char
    TabManagerStatsCollector::kHistogramBackgroundTabOpeningSwitchToTab[] =
        "TabManager.BackgroundTabOpening.SwitchToTab";

// Static
const char
    TabManagerStatsCollector::kHistogramSessionRestoreTabSwitchLoadTime[] =
        "TabManager.Experimental.SessionRestore.TabSwitchLoadTime";

// Static
const char TabManagerStatsCollector::
    kHistogramBackgroundTabOpeningTabSwitchLoadTime[] =
        "TabManager.Experimental.BackgroundTabOpening.TabSwitchLoadTime";

}  // namespace resource_coordinator
