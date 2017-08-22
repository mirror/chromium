// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_manager_stats_collector.h"

#include <cstdint>
#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "chrome/browser/resource_coordinator/tab_manager.h"
#include "chrome/browser/resource_coordinator/tab_manager_web_contents_data.h"
#include "chrome/browser/sessions/session_restore.h"
#include "content/public/browser/swap_metrics_driver.h"

namespace resource_coordinator {

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

TabManagerStatsCollector::TabManagerStatsCollector(TabManager* tab_manager)
    : tab_manager_(tab_manager),
      is_session_restore_loading_tabs_(false),
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
    content::WebContents* contents) const {
  auto* data = TabManager::WebContentsData::FromWebContents(contents);
  DCHECK(data);

  if (tab_manager_->IsSessionRestoreLoadingTabs()) {
    UMA_HISTOGRAM_ENUMERATION("TabManager.SessionRestore.SwitchToTab",
                              data->tab_loading_state(), TAB_LOADING_STATE_MAX);
  }
  if (is_in_background_tab_opening_session_) {
    UMA_HISTOGRAM_ENUMERATION("TabManager.BackgroundTabOpening.SwitchToTab",
                              data->tab_loading_state(), TAB_LOADING_STATE_MAX);
  }
}

void TabManagerStatsCollector::RecordExpectedTaskQueueingDuration(
    content::WebContents* contents,
    base::TimeDelta queueing_time) {
  if (!contents->IsVisible())
    return;

  if (tab_manager_->IsSessionRestoreLoadingTabs()) {
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
  if (background_tab_opening_swap_metrics_driver_)
    background_tab_opening_swap_metrics_driver_->InitializeMetrics();
  is_in_background_tab_opening_session_ = true;
}

void TabManagerStatsCollector::OnBackgroundTabOpeningSessionEnded() {
  DCHECK(is_in_background_tab_opening_session_);
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

// static
const char TabManagerStatsCollector::
    kHistogramSessionRestoreForegroundTabExpectedTaskQueueingDuration[] =
        "TabManager.SessionRestore.ForegroundTab.ExpectedTaskQueueingDuration";

// static
const char TabManagerStatsCollector::
    kHistogramBackgroundTabOpeningForegroundTabExpectedTaskQueueingDuration[] =
        "TabManager.BackgroundTabOpening.ForegroundTab."
        "ExpectedTaskQueueingDuration";

}  // namespace resource_coordinator
