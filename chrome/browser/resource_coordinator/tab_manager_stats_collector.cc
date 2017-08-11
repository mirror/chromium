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

namespace {

const char* kSessionRestoreName = "SessionRestore";
const char* kBackgroundTabOpenName = "BackgroundTabOpen";

}  // namespace

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
      EventType type)
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
  const EventType event_type_;
};

TabManagerStatsCollector::TabManagerStatsCollector(TabManager* tab_manager)
    : tab_manager_(tab_manager), is_session_restore_loading_tabs_(false) {
  session_restore_swap_metrics_driver_ = content::SwapMetricsDriver::Create(
      base::WrapUnique<content::SwapMetricsDriver::Delegate>(
          new SwapMetricsDelegate(this, kSessionRestore)),
      base::TimeDelta::FromSeconds(0));
  SessionRestore::AddObserver(this);

  background_tab_open_swap_metrics_driver_ = content::SwapMetricsDriver::Create(
      base::WrapUnique<content::SwapMetricsDriver::Delegate>(
          new SwapMetricsDelegate(this, kBackgroundTabOpen)),
      base::TimeDelta::FromSeconds(0));
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
  if (tab_manager_->IsLoadingBackgroundTabs()) {
    UMA_HISTOGRAM_ENUMERATION("TabManager.BackgroundTabOpen.SwitchToTab",
                              data->tab_loading_state(), TAB_LOADING_STATE_MAX);
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

void TabManagerStatsCollector::OnStartedLoadingBackgroundTabs() {
  background_tab_stats_.Reset();

  if (background_tab_open_swap_metrics_driver_)
    background_tab_open_swap_metrics_driver_->InitializeMetrics();
}

void TabManagerStatsCollector::OnFinishedLoadingBackgroundTabs() {
  UMA_HISTOGRAM_COUNTS_100("TabManager.BackgroundTabOpen.TabCount",
                           background_tab_stats_.tab_count);
  UMA_HISTOGRAM_COUNTS_100("TabManager.BackgroundTabOpen.TabsPaused",
                           background_tab_stats_.tabs_paused);
  UMA_HISTOGRAM_COUNTS_100("TabManager.BackgroundTabOpen.TabsLoadAutoStarted",
                           background_tab_stats_.tabs_load_auto_started);
  UMA_HISTOGRAM_COUNTS_100("TabManager.BackgroundTabOpen.TabsLoadUserInitiated",
                           background_tab_stats_.tabs_load_user_initiated);

  if (background_tab_open_swap_metrics_driver_)
    background_tab_open_swap_metrics_driver_->UpdateMetrics();
}

void TabManagerStatsCollector::OnSwapInCount(EventType type,
                                             uint64_t count,
                                             base::TimeDelta interval) {
  DCHECK(
      (type == kSessionRestore && is_session_restore_loading_tabs_) ||
      (type == kBackgroundTabOpen && tab_manager_->IsLoadingBackgroundTabs()));
  UMA_HISTOGRAM_COUNTS_10000(
      "TabManager." + GetEventName(type) + ".SwapInPerSecond",
      static_cast<double>(count) / interval.InSecondsF());
}

void TabManagerStatsCollector::OnSwapOutCount(EventType type,
                                              uint64_t count,
                                              base::TimeDelta interval) {
  DCHECK(
      (type == kSessionRestore && is_session_restore_loading_tabs_) ||
      (type == kBackgroundTabOpen && tab_manager_->IsLoadingBackgroundTabs()));
  UMA_HISTOGRAM_COUNTS_10000(
      "TabManager." + GetEventName(type) + ".SwapOutPerSecond",
      static_cast<double>(count) / interval.InSecondsF());
}

void TabManagerStatsCollector::OnDecompressedPageCount(
    EventType type,
    uint64_t count,
    base::TimeDelta interval) {
  DCHECK(
      (type == kSessionRestore && is_session_restore_loading_tabs_) ||
      (type == kBackgroundTabOpen && tab_manager_->IsLoadingBackgroundTabs()));
  UMA_HISTOGRAM_COUNTS_10000(
      "TabManager." + GetEventName(type) + ".DecompressedPagesPerSecond",
      static_cast<double>(count) / interval.InSecondsF());
}

void TabManagerStatsCollector::OnCompressedPageCount(EventType type,
                                                     uint64_t count,
                                                     base::TimeDelta interval) {
  DCHECK(
      (type == kSessionRestore && is_session_restore_loading_tabs_) ||
      (type == kBackgroundTabOpen && tab_manager_->IsLoadingBackgroundTabs()));
  UMA_HISTOGRAM_COUNTS_10000(
      "TabManager." + GetEventName(type) + ".CompressedPagesPerSecond",
      static_cast<double>(count) / interval.InSecondsF());
}

void TabManagerStatsCollector::OnUpdateMetricsFailed(EventType type) {
  // If either InitializeMetrics() or UpdateMetrics() fails, it's unlikely an
  // error that can be recovered from, in which case we don't collect swap
  // metrics for session restore.
  switch (type) {
    case kSessionRestore:
      session_restore_swap_metrics_driver_.reset();
      break;
    case kBackgroundTabOpen:
      background_tab_open_swap_metrics_driver_.reset();
      break;
    default:
      NOTREACHED();
  }
}

std::string TabManagerStatsCollector::GetEventName(EventType type) const {
  switch (type) {
    case kSessionRestore:
      return kSessionRestoreName;
    case kBackgroundTabOpen:
      return kBackgroundTabOpenName;
    default:
      NOTREACHED();
  }
}

}  // namespace resource_coordinator
