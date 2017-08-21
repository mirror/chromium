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
const int64_t kDailyEventIntervalCheckInS = 60 * 5;
static TabStatsTracker* g_instance = nullptr;

}  // namespace

// static
const char TabStatsTracker::kTabStatsDailyEventHistogramName[] =
    "Tabs.TabsStats.DailyEventInteral";
const char TabStatsTracker::UmaStatsReportingDelegate::
    kNumberOfTabsOnResumeHistogramName[] = "Tabs.NumberOfTabsOnResume";
const char
    TabStatsTracker::UmaStatsReportingDelegate::kMaxTabsInADayHistogramName[] =
        "Tabs.MaxTabsInADay";
const char TabStatsTracker::UmaStatsReportingDelegate::
    kMaxTabsPerBrowserInADayHistogramName[] = "Tabs.MaxTabsPerBrowserInADay";
const char TabStatsTracker::UmaStatsReportingDelegate::
    kMaxBrowsersInADayHistogramName[] = "Tabs.MaxBrowsersInADay";

// static
void TabStatsTracker::Initialize(PrefService* pref_service) {
  if (g_instance == nullptr) {
    g_instance = new TabStatsTracker(pref_service);
  }
}

// static
TabStatsTracker* TabStatsTracker::GetInstance() {
  CHECK_NE(nullptr, g_instance);
  return g_instance;
}

const TabsStatsDataStore::TabsStats& TabStatsTracker::tab_stats() const {
  return tab_stats_data_store_->tab_stats();
}

void TabStatsTracker::TabStatsDailyObserver::OnDailyEvent() {
  reporting_delegate_->ReportDailyMetrics(data_store_->tab_stats());
  data_store_->ResetMaximumsToCurrentState();
}

TabStatsTracker::TabStatsTracker(PrefService* pref_service)
    : reporting_delegate_(base::MakeUnique<UmaStatsReportingDelegate>()),
      tab_stats_data_store_(base::MakeUnique<TabsStatsDataStore>(pref_service)),
      daily_event_(
          base::MakeUnique<DailyEvent>(pref_service,
                                       prefs::kTabStatsDailySample,
                                       kTabStatsDailyEventHistogramName)) {
  DCHECK_NE(nullptr, pref_service);
  // Get the list of existing browsers/tabs. There shouldn't be any if this is
  // initialized at startup but this will ensure that the count stay accurate if
  // the initialization gets moved to after the creation of the first tab.
  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list) {
    browser->tab_strip_model()->AddObserver(this);
    tab_stats_data_store_->OnBrowsersAdded(1);
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
  for (Browser* browser : *browser_list) {
    browser->tab_strip_model()->RemoveObserver(this);
  }
  browser_list->RemoveObserver(this);

  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  if (power_monitor != nullptr)
    power_monitor->RemoveObserver(this);
}

void TabStatsTracker::OnBrowserAdded(Browser* browser) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  tab_stats_data_store_->OnBrowsersAdded(1);
  browser->tab_strip_model()->AddObserver(this);
}

void TabStatsTracker::OnBrowserRemoved(Browser* browser) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  tab_stats_data_store_->OnBrowsersRemoved(1);
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

TabsStatsDataStore::TabsStats::TabsStats()
    : total_tab_count(0U),
      total_tab_count_max(0U),
      max_tab_per_window(0U),
      browser_count(0U),
      browser_count_max(0U) {}

TabsStatsDataStore::TabsStatsDataStore(PrefService* pref_service)
    : pref_service_(pref_service) {
  tab_stats_.total_tab_count_max =
      pref_service_->GetInteger(prefs::kTabStatsTotalTabCountMax);
  tab_stats_.max_tab_per_window =
      pref_service_->GetInteger(prefs::kTabStatsMaxTabsPerWindow);
  tab_stats_.browser_count_max =
      pref_service_->GetInteger(prefs::kTabStatsBrowserCountMax);
}

void TabsStatsDataStore::OnBrowsersAdded(size_t browser_count) {
  tab_stats_.browser_count += browser_count;
  UpdateBrowserCountMaxIfNeeded();
}

void TabsStatsDataStore::OnBrowsersRemoved(size_t browser_count) {
  tab_stats_.browser_count -= browser_count;
}

void TabsStatsDataStore::OnTabsAdded(size_t tab_count) {
  tab_stats_.total_tab_count += tab_count;
  UpdateTotalTabCountMaxIfNeeded();
}

void TabsStatsDataStore::OnTabsRemoved(size_t tab_count) {
  tab_stats_.total_tab_count -= tab_count;
}

void TabsStatsDataStore::UpdateMaxTabsPerWindowIfNeeded(size_t value) {
  if (value <= tab_stats_.max_tab_per_window)
    return;
  tab_stats_.max_tab_per_window = value;
  pref_service_->SetInteger(prefs::kTabStatsMaxTabsPerWindow, value);
}

void TabsStatsDataStore::ResetMaximumsToCurrentState() {
  tab_stats_.max_tab_per_window = 0;
  tab_stats_.browser_count_max = 0;
  tab_stats_.total_tab_count_max = 0;
  UpdateTotalTabCountMaxIfNeeded();
  UpdateBrowserCountMaxIfNeeded();

  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list) {
    UpdateMaxTabsPerWindowIfNeeded(
        static_cast<size_t>(browser->tab_strip_model()->count()));
  }
}

void TabsStatsDataStore::UpdateTotalTabCountMaxIfNeeded() {
  if (tab_stats_.total_tab_count <= tab_stats_.total_tab_count_max)
    return;
  tab_stats_.total_tab_count_max = tab_stats_.total_tab_count;
  pref_service_->SetInteger(prefs::kTabStatsTotalTabCountMax,
                            tab_stats_.total_tab_count_max);
}

void TabsStatsDataStore::UpdateBrowserCountMaxIfNeeded() {
  if (tab_stats_.browser_count <= tab_stats_.browser_count_max)
    return;
  tab_stats_.browser_count_max = tab_stats_.browser_count;
  pref_service_->SetInteger(prefs::kTabStatsBrowserCountMax,
                            tab_stats_.browser_count_max);
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
  UMA_HISTOGRAM_COUNTS_10000(kMaxTabsPerBrowserInADayHistogramName,
                             tab_stats.max_tab_per_window);
  UMA_HISTOGRAM_COUNTS_10000(kMaxBrowsersInADayHistogramName,
                             tab_stats.browser_count_max);
}

}  // namespace metrics
