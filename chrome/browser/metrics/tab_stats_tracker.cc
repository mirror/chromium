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

namespace metrics {

// static
const char TabStatsTracker::UmaStatsReportingDelegate::kNumberOfTabsOnResume[] =
    "TabStats.NumberOfTabsOnResume";

TabStatsTracker::TabStats::TabStats()
    : total_tabs_count(0U),
      total_tabs_count_max(0U),
      max_tabs_per_window(0U),
      browser_count(0U),
      browser_count_max(0U) {}

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
    : reporting_delegate_(
          base::MakeUnique<
              metrics::TabStatsTracker::UmaStatsReportingDelegate>()) {
  // Get the list of existing browsers/tabs. There shouldn't be any if this is
  // initialized at startup but this will ensure that the count stay accurate if
  // the initialization gets moved to after the creation of the first tab.
  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list) {
    ++tab_stats_.browser_count;
    browser->tab_strip_model()->AddObserver(this);
    tab_stats_.total_tabs_count += browser->tab_strip_model()->count();
    if (static_cast<size_t>(browser->tab_strip_model()->count()) >
        tab_stats_.max_tabs_per_window) {
      tab_stats_.max_tabs_per_window = browser->tab_strip_model()->count();
    }
  }
  tab_stats_.browser_count_max = tab_stats_.browser_count;
  tab_stats_.total_tabs_count_max = tab_stats_.total_tabs_count;

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
    tab_stats_.total_tabs_count -= browser->tab_strip_model()->count();
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
  if (tab_stats_.browser_count > tab_stats_.browser_count_max) {
    tab_stats_.browser_count_max = tab_stats_.browser_count;
  }
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
  ++tab_stats_.total_tabs_count;
  if (tab_stats_.total_tabs_count > tab_stats_.total_tabs_count_max) {
    tab_stats_.total_tabs_count_max = tab_stats_.total_tabs_count;
  }
  if (static_cast<size_t>(model->count()) > tab_stats_.max_tabs_per_window) {
    tab_stats_.max_tabs_per_window = static_cast<size_t>(model->count());
  }
}

void TabStatsTracker::TabClosingAt(TabStripModel* model,
                                   content::WebContents* web_contents,
                                   int index) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  --tab_stats_.total_tabs_count;
}

void TabStatsTracker::OnResume() {
  reporting_delegate_->ReportTabCountOnResume(tab_stats_.total_tabs_count);
}

void TabStatsTracker::UmaStatsReportingDelegate::ReportTabCountOnResume(
    size_t tab_count) {
  UMA_HISTOGRAM_COUNTS_100(kNumberOfTabsOnResume, tab_count);
}

}  // namespace metrics
