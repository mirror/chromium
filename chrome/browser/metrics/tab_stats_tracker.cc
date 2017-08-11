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

TabStatsTracker::TabStats::TabStats()
    : total_tab_count(0U),
      total_tab_count_max(0U),
      max_tab_per_window(0U),
      browser_count(0U),
      browser_count_max(0U) {
  if (g_browser_process != nullptr) {
    total_tab_count_max = g_browser_process->local_state()->GetInteger(
        prefs::kTabStatsTotalTabCountMax);
    max_tab_per_window = g_browser_process->local_state()->GetInteger(
        prefs::kTabStatsMaxTabsPerWindow);
    browser_count_max = g_browser_process->local_state()->GetInteger(
        prefs::kTabStatsBrowserCountMax);
  }
}

void TabStatsTracker::TabStats::UpdateTotalTabCountMaxIfNeeded() {
  if (total_tab_count <= total_tab_count_max)
    return;
  total_tab_count_max = total_tab_count;
  if (g_browser_process != nullptr) {
    g_browser_process->local_state()->SetInteger(
        prefs::kTabStatsTotalTabCountMax, total_tab_count_max);
  }
}
void TabStatsTracker::TabStats::UpdateMaxTabsPerWindowIfNeeded(size_t value) {
  if (value <= max_tab_per_window)
    return;
  max_tab_per_window = value;
  if (g_browser_process != nullptr) {
    g_browser_process->local_state()->SetInteger(
        prefs::kTabStatsMaxTabsPerWindow, value);
  }
}
void TabStatsTracker::TabStats::UpdateBrowserCountMaxIfNeeded() {
  if (browser_count <= browser_count_max)
    return;
  browser_count_max = browser_count;
  if (g_browser_process != nullptr) {
    g_browser_process->local_state()->SetInteger(
        prefs::kTabStatsBrowserCountMax, browser_count_max);
  }
}

// static
void TabStatsTracker::Initialize() {
  // Calls GetInstance() to initialize the static instance.
  GetInstance();
}

// static
TabStatsTracker* TabStatsTracker::GetInstance() {
  static TabStatsTracker* instance = new TabStatsTracker();
  return instance;
}

TabStatsTracker::TabStatsTracker()
    : reporting_delegate_(base::MakeUnique<UmaStatsReportingDelegate>()) {
  // Get the list of existing browsers/tabs. There shouldn't be any if this is
  // initialized at startup but this will ensure that the count stay accurate if
  // the initialization gets moved to after the creation of the first tab.
  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list) {
    ++tab_stats_.browser_count;
    browser->tab_strip_model()->AddObserver(this);
    tab_stats_.total_tab_count += browser->tab_strip_model()->count();
    tab_stats_.UpdateMaxTabsPerWindowIfNeeded(
        static_cast<size_t>(browser->tab_strip_model()->count()));
  }
  tab_stats_.UpdateBrowserCountMaxIfNeeded();
  tab_stats_.UpdateTotalTabCountMaxIfNeeded();

  browser_list->AddObserver(this);
  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  if (power_monitor != nullptr)
    power_monitor->AddObserver(this);
}

TabStatsTracker::~TabStatsTracker() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list) {
    --tab_stats_.browser_count;
    browser->tab_strip_model()->RemoveObserver(this);
    tab_stats_.total_tab_count -= browser->tab_strip_model()->count();
  }
  browser_list->RemoveObserver(this);

  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  if (power_monitor != nullptr)
    power_monitor->RemoveObserver(this);

  ::memset(reinterpret_cast<uint8_t*>(&tab_stats_), 0, sizeof(TabStats));
}

void TabStatsTracker::OnBrowserAdded(Browser* browser) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ++tab_stats_.browser_count;

  tab_stats_.UpdateBrowserCountMaxIfNeeded();

  browser->tab_strip_model()->AddObserver(this);
}

void TabStatsTracker::OnBrowserRemoved(Browser* browser) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  --tab_stats_.browser_count;
  browser->tab_strip_model()->RemoveObserver(this);
}

void TabStatsTracker::TabInsertedAt(TabStripModel* model,
                                    content::WebContents* web_contents,
                                    int index,
                                    bool foreground) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ++tab_stats_.total_tab_count;

  tab_stats_.UpdateTotalTabCountMaxIfNeeded();
  tab_stats_.UpdateMaxTabsPerWindowIfNeeded(
      static_cast<size_t>(model->count()));
}

void TabStatsTracker::TabClosingAt(TabStripModel* model,
                                   content::WebContents* web_contents,
                                   int index) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  --tab_stats_.total_tab_count;
}

void TabStatsTracker::OnResume() {
  reporting_delegate_->ReportTabCountOnResume(tab_stats_.total_tab_count);
}

void TabStatsTracker::UmaStatsReportingDelegate::ReportTabCountOnResume(
    size_t tab_count) {
  UMA_HISTOGRAM_COUNTS_10000(kNumberOfTabsOnResumeHistogramName, tab_count);
}

}  // namespace metrics
