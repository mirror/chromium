// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/tab_stats_tracker.h"

#include <algorithm>

#include "base/metrics/histogram_macros.h"
#include "base/power_monitor/power_monitor.h"
#include "chrome/browser/background/background_mode_manager.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_service.h"

namespace metrics {

namespace {

// The interval at which the DailyEvent::CheckInterval function should be
// called.
const int64_t kDailyEventIntervalCheckInS = 60 * 2;

// The global TabStatsTracker instance.
std::unique_ptr<TabStatsTracker> g_instance;

}  // namespace

// static
const char TabStatsTracker::kTabStatsDailyEventHistogramName[] =
    "Tabs.TabsStatsDailyEventInteral";
const char TabStatsTracker::UmaStatsReportingDelegate::
    kNumberOfTabsOnResumeHistogramName[] = "Tabs.NumberOfTabsOnResume";
const char
    TabStatsTracker::UmaStatsReportingDelegate::kMaxTabsInADayHistogramName[] =
        "Tabs.MaxTabsInADay";
const char TabStatsTracker::UmaStatsReportingDelegate::
    kMaxTabsPerWindowInADayHistogramName[] = "Tabs.MaxTabsPerWindowInADay";
const char TabStatsTracker::UmaStatsReportingDelegate::
    kMaxWindowsInADayHistogramName[] = "Tabs.MaxWindowsInADay";

TabsStatsDataStore::TabsStats::TabsStats()
    : total_tab_count(0U),
      total_tab_count_max(0U),
      max_tab_per_window(0U),
      window_count(0U),
      window_count_max(0U) {}

TabsStatsDataStore::TabsStatsDataStore(PrefService* pref_service)
    : pref_service_(pref_service) {
  CHECK(pref_service != nullptr);
  tab_stats_.total_tab_count_max =
      pref_service->GetInteger(prefs::kTabStatsTotalTabCountMax);
  tab_stats_.max_tab_per_window =
      pref_service->GetInteger(prefs::kTabStatsMaxTabsPerWindow);
  tab_stats_.window_count_max =
      pref_service->GetInteger(prefs::kTabStatsWindowCountMax);
}

void TabsStatsDataStore::OnWindowAdded() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  tab_stats_.window_count++;
  UpdateWindowCountMaxIfNeeded();
}

void TabsStatsDataStore::OnWindowRemoved() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_GT(tab_stats_.window_count, 0U);
  tab_stats_.window_count--;
}

void TabsStatsDataStore::OnTabsAdded(size_t tab_count) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  tab_stats_.total_tab_count += tab_count;
  UpdateTotalTabCountMaxIfNeeded();
}

void TabsStatsDataStore::OnTabsRemoved(size_t tab_count) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_LE(tab_count, tab_stats_.total_tab_count);
  tab_stats_.total_tab_count -= tab_count;
}

void TabsStatsDataStore::UpdateMaxTabsPerWindowIfNeeded(size_t value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (value <= tab_stats_.max_tab_per_window)
    return;
  tab_stats_.max_tab_per_window = value;
  pref_service_->SetInteger(prefs::kTabStatsMaxTabsPerWindow, value);
}

void TabsStatsDataStore::ResetMaximumsToCurrentState() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Set the maximums to 0 and call the Update* functions to reset it to the
  // current value and update the pref registry.
  tab_stats_.max_tab_per_window = 0;
  tab_stats_.window_count_max = 0;
  tab_stats_.total_tab_count_max = 0;
  UpdateTotalTabCountMaxIfNeeded();
  UpdateWindowCountMaxIfNeeded();

  // Iterates over the list of browsers to find the one with the maximum number
  // of tabs opened.
  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list) {
    UpdateMaxTabsPerWindowIfNeeded(
        static_cast<size_t>(browser->tab_strip_model()->count()));
  }
}

void TabsStatsDataStore::UpdateTotalTabCountMaxIfNeeded() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (tab_stats_.total_tab_count <= tab_stats_.total_tab_count_max)
    return;
  tab_stats_.total_tab_count_max = tab_stats_.total_tab_count;
  pref_service_->SetInteger(prefs::kTabStatsTotalTabCountMax,
                            tab_stats_.total_tab_count_max);
}

void TabsStatsDataStore::UpdateWindowCountMaxIfNeeded() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (tab_stats_.window_count <= tab_stats_.window_count_max)
    return;
  tab_stats_.window_count_max = tab_stats_.window_count;
  pref_service_->SetInteger(prefs::kTabStatsWindowCountMax,
                            tab_stats_.window_count_max);
}

const TabsStatsDataStore::TabsStats& TabStatsTracker::tab_stats() const {
  return tab_stats_data_store_->tab_stats();
}

TabStatsTracker::TabStatsTracker(PrefService* pref_service)
    : reporting_delegate_(base::MakeUnique<UmaStatsReportingDelegate>()),
      tab_stats_data_store_(base::MakeUnique<TabsStatsDataStore>(pref_service)),
      daily_event_(
          base::MakeUnique<DailyEvent>(pref_service,
                                       prefs::kTabStatsDailySample,
                                       kTabStatsDailyEventHistogramName)) {
  DCHECK_NE(nullptr, pref_service);
  // Get the list of existing windows/tabs. There shouldn't be any if this is
  // initialized at startup but this will ensure that the counts stay accurate
  // if the initialization gets moved to after the creation of the first tab.
  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list) {
    browser->tab_strip_model()->AddObserver(this);
    tab_stats_data_store_->OnWindowAdded();
    tab_stats_data_store_->OnTabsAdded(browser->tab_strip_model()->count());
    tab_stats_data_store_->UpdateMaxTabsPerWindowIfNeeded(
        static_cast<size_t>(browser->tab_strip_model()->count()));
  }

  browser_list->AddObserver(this);
  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  if (power_monitor != nullptr)
    power_monitor->AddObserver(this);

  daily_event_->AddObserver(base::MakeUnique<TabStatsDailyObserver>(
      reporting_delegate_.get(), tab_stats_data_store_.get()));
  timer_.Start(FROM_HERE,
               base::TimeDelta::FromSeconds(kDailyEventIntervalCheckInS),
               daily_event_.get(), &DailyEvent::CheckInterval);
}

TabStatsTracker::~TabStatsTracker() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  timer_.Stop();
  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list)
    browser->tab_strip_model()->RemoveObserver(this);

  browser_list->RemoveObserver(this);

  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  if (power_monitor != nullptr)
    power_monitor->RemoveObserver(this);
}

// static
void TabStatsTracker::SetInstance(std::unique_ptr<TabStatsTracker> instance) {
  DCHECK_EQ(nullptr, g_instance.get());
  g_instance = std::move(instance);
}

TabStatsTracker* TabStatsTracker::GetInstance() {
  return g_instance.get();
}

void TabStatsTracker::TabStatsDailyObserver::OnDailyEvent() {
  reporting_delegate_->ReportDailyMetrics(data_store_->tab_stats());
  data_store_->ResetMaximumsToCurrentState();
}

void TabStatsTracker::OnBrowserAdded(Browser* browser) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  tab_stats_data_store_->OnWindowAdded();
  browser->tab_strip_model()->AddObserver(this);
}

void TabStatsTracker::OnBrowserRemoved(Browser* browser) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  tab_stats_data_store_->OnWindowRemoved();
  browser->tab_strip_model()->RemoveObserver(this);
}

void TabStatsTracker::TabInsertedAt(TabStripModel* model,
                                    content::WebContents* web_contents,
                                    int index,
                                    bool foreground) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  tab_stats_data_store_->OnTabsAdded(1);

  tab_stats_data_store_->UpdateMaxTabsPerWindowIfNeeded(
      static_cast<size_t>(model->count()));
}

void TabStatsTracker::TabClosingAt(TabStripModel* model,
                                   content::WebContents* web_contents,
                                   int index) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  tab_stats_data_store_->OnTabsRemoved(1);
}

void TabStatsTracker::OnResume() {
  reporting_delegate_->ReportTabCountOnResume(
      tab_stats_data_store_->tab_stats().total_tab_count);
}

void TabStatsTracker::UmaStatsReportingDelegate::ReportTabCountOnResume(
    size_t tab_count) {
  // Don't report the number of tabs on resume if Chrome is running in
  // background with no visible window.
  if (g_browser_process && g_browser_process->background_mode_manager()
                               ->IsBackgroundWithoutWindows()) {
    return;
  }
  UMA_HISTOGRAM_COUNTS_10000(kNumberOfTabsOnResumeHistogramName, tab_count);
}

void TabStatsTracker::UmaStatsReportingDelegate::ReportDailyMetrics(
    const TabsStatsDataStore::TabsStats& tab_stats) {
  UMA_HISTOGRAM_COUNTS_10000(kMaxTabsInADayHistogramName,
                             tab_stats.total_tab_count_max);
  UMA_HISTOGRAM_COUNTS_10000(kMaxTabsPerWindowInADayHistogramName,
                             tab_stats.max_tab_per_window);
  UMA_HISTOGRAM_COUNTS_10000(kMaxWindowsInADayHistogramName,
                             tab_stats.window_count_max);
}

}  // namespace metrics
