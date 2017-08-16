// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/tab_stats_tracker.h"

#include <algorithm>

#include "base/metrics/histogram_macros.h"
#include "base/power_monitor/power_monitor.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_service.h"

namespace metrics {

// static
const char TabStatsTracker::UmaStatsReportingDelegate::
    kNumberOfTabsOnResumeHistogramName[] = "Tabs.NumberOfTabsOnResume";

static TabStatsTracker* g_instance = nullptr;

// static
void TabStatsTracker::Initialize(PrefService* pref_service) {
  if (g_instance == nullptr) {
    g_instance = new TabStatsTracker(pref_service);
  }
}

// static
TabStatsTracker* TabStatsTracker::GetInstance() {
  // static TabStatsTracker* instance = new TabStatsTracker();
  CHECK_NE(nullptr, g_instance);
  return g_instance;
}

TabStatsTracker::TabStatsTracker(PrefService* pref_service)
    : reporting_delegate_(base::MakeUnique<UmaStatsReportingDelegate>()),
      tab_stats_data_store_(
          base::MakeUnique<TabsStatsDataStore>(pref_service)) {
  DCHECK_NE(nullptr, pref_service);
  // Get the list of existing browsers/tabs. There shouldn't be any if this is
  // initialized at startup but this will ensure that the count stay accurate if
  // the initialization gets moved to after the creation of the first tab.
  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list) {
    ++tab_stats_data_store_->tab_stats().browser_count;
    browser->tab_strip_model()->AddObserver(this);
    tab_stats_data_store_->tab_stats().total_tab_count +=
        browser->tab_strip_model()->count();
    tab_stats_data_store_->UpdateMaxTabsPerWindowIfNeeded(
        static_cast<size_t>(browser->tab_strip_model()->count()));
  }
  tab_stats_data_store_->UpdateBrowserCountMaxIfNeeded();
  tab_stats_data_store_->UpdateTotalTabCountMaxIfNeeded();

  browser_list->AddObserver(this);
  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  if (power_monitor != nullptr)
    power_monitor->AddObserver(this);
}

TabStatsTracker::~TabStatsTracker() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list) {
    --tab_stats_data_store_->tab_stats().browser_count;
    browser->tab_strip_model()->RemoveObserver(this);
    tab_stats_data_store_->tab_stats().total_tab_count -=
        browser->tab_strip_model()->count();
  }
  browser_list->RemoveObserver(this);

  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  if (power_monitor != nullptr)
    power_monitor->RemoveObserver(this);
}

void TabStatsTracker::OnBrowserAdded(Browser* browser) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ++tab_stats_data_store_->tab_stats().browser_count;

  tab_stats_data_store_->UpdateBrowserCountMaxIfNeeded();

  browser->tab_strip_model()->AddObserver(this);
}

void TabStatsTracker::OnBrowserRemoved(Browser* browser) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  --tab_stats_data_store_->tab_stats().browser_count;
  browser->tab_strip_model()->RemoveObserver(this);
}

void TabStatsTracker::TabInsertedAt(TabStripModel* model,
                                    content::WebContents* web_contents,
                                    int index,
                                    bool foreground) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ++tab_stats_data_store_->tab_stats().total_tab_count;

  tab_stats_data_store_->UpdateTotalTabCountMaxIfNeeded();
  tab_stats_data_store_->UpdateMaxTabsPerWindowIfNeeded(
      static_cast<size_t>(model->count()));
}

void TabStatsTracker::TabClosingAt(TabStripModel* model,
                                   content::WebContents* web_contents,
                                   int index) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  --tab_stats_data_store_->tab_stats().total_tab_count;
}

void TabStatsTracker::OnResume() {
  reporting_delegate_->ReportTabCountOnResume(
      tab_stats_data_store_->tab_stats().total_tab_count);
}

void TabStatsTracker::UmaStatsReportingDelegate::ReportTabCountOnResume(
    size_t tab_count) {
  UMA_HISTOGRAM_COUNTS_10000(kNumberOfTabsOnResumeHistogramName, tab_count);
}

TabStatsTracker::TabsStatsDataStore::TabStats::TabStats()
    : total_tab_count(0U),
      total_tab_count_max(0U),
      max_tab_per_window(0U),
      browser_count(0U),
      browser_count_max(0U) {}

void TabStatsTracker::TabsStatsDataStore::UpdatePrefs() {
  tab_stats_.total_tab_count_max =
      pref_service_->GetInteger(prefs::kTabStatsTotalTabCountMax);
  tab_stats_.max_tab_per_window =
      pref_service_->GetInteger(prefs::kTabStatsMaxTabsPerWindow);
  tab_stats_.browser_count_max =
      pref_service_->GetInteger(prefs::kTabStatsBrowserCountMax);
}

void TabStatsTracker::TabsStatsDataStore::UpdateTotalTabCountMaxIfNeeded() {
  if (tab_stats_.total_tab_count <= tab_stats_.total_tab_count_max)
    return;
  tab_stats_.total_tab_count_max = tab_stats_.total_tab_count;
  pref_service_->SetInteger(prefs::kTabStatsTotalTabCountMax,
                            tab_stats_.total_tab_count_max);
}

void TabStatsTracker::TabsStatsDataStore::UpdateMaxTabsPerWindowIfNeeded(
    size_t value) {
  if (value <= tab_stats_.max_tab_per_window)
    return;
  tab_stats_.max_tab_per_window = value;
  pref_service_->SetInteger(prefs::kTabStatsMaxTabsPerWindow, value);
}

void TabStatsTracker::TabsStatsDataStore::UpdateBrowserCountMaxIfNeeded() {
  if (tab_stats_.browser_count <= tab_stats_.browser_count_max)
    return;
  tab_stats_.browser_count_max = tab_stats_.browser_count;
  pref_service_->SetInteger(prefs::kTabStatsBrowserCountMax,
                            tab_stats_.browser_count_max);
}

}  // namespace metrics
