// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/tab_stats_tracker.h"

#include <algorithm>
#include <utility>

#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/power_monitor/power_monitor.h"
#include "base/strings/stringprintf.h"
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
constexpr base::TimeDelta kDailyEventIntervalTimeDelta =
    base::TimeDelta::FromMilliseconds(60 * 30);

// The global TabStatsTracker instance.
TabStatsTracker* g_instance = nullptr;

// The intervals at which we report the number of unused tabs, in seconds.
const size_t kIntervalsInSec[] = {
    30,           // 30 seconds
    60,           // 60 seconds
    60 * 10,      // 10 minutes
    60 * 60,      // 1 hour
    60 * 60 * 5,  // 5 hours
    60 * 60 * 10  // 10 hours
};

}  // namespace

// static
const char TabStatsTracker::kTabStatsDailyEventHistogramName[] =
    "Tabs.TabsStatsDailyEventInterval";
const char TabStatsTracker::UmaStatsReportingDelegate::
    kNumberOfTabsOnResumeHistogramName[] = "Tabs.NumberOfTabsOnResume";
const char
    TabStatsTracker::UmaStatsReportingDelegate::kMaxTabsInADayHistogramName[] =
        "Tabs.MaxTabsInADay";
const char TabStatsTracker::UmaStatsReportingDelegate::
    kMaxTabsPerWindowInADayHistogramName[] = "Tabs.MaxTabsPerWindowInADay";
const char TabStatsTracker::UmaStatsReportingDelegate::
    kMaxWindowsInADayHistogramName[] = "Tabs.MaxWindowsInADay";
const char TabStatsTracker::UmaStatsReportingDelegate::
    kUnusedTabsInIntervalHistogramNameBase[] = "Tabs.UnusedInInterval.Count";

const TabStatsDataStore::TabsStats& TabStatsTracker::tab_stats() const {
  return tab_stats_data_store_->tab_stats();
}

TabStatsTracker::TabStatsTracker(PrefService* pref_service)
    : reporting_delegate_(base::MakeUnique<UmaStatsReportingDelegate>()),
      tab_stats_data_store_(base::MakeUnique<TabStatsDataStore>(pref_service)),
      daily_event_(
          base::MakeUnique<DailyEvent>(pref_service,
                                       prefs::kTabStatsDailySample,
                                       kTabStatsDailyEventHistogramName)) {
  DCHECK(pref_service != nullptr);
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
    for (int i = 0; i < browser->tab_strip_model()->count(); ++i)
      existing_tabs_.insert(browser->tab_strip_model()->GetWebContentsAt(i));
    active_tabs_.insert(browser->tab_strip_model()->GetActiveWebContents());
  }

  browser_list->AddObserver(this);
  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  if (power_monitor != nullptr)
    power_monitor->AddObserver(this);

  daily_event_->AddObserver(base::MakeUnique<TabStatsDailyObserver>(
      reporting_delegate_.get(), tab_stats_data_store_.get()));
  // Call the CheckInterval method to see if the data need to be immediately
  // reported.
  daily_event_->CheckInterval();
  timer_.Start(FROM_HERE, kDailyEventIntervalTimeDelta, daily_event_.get(),
               &DailyEvent::CheckInterval);

  interval_maps_.reserve(arraysize(kIntervalsInSec));
  // Initialize the interval maps and timers associated with them.
  for (size_t i = 0; i < arraysize(kIntervalsInSec); ++i) {
    // Creates the interval and initialize its data.
    std::unique_ptr<TabsStateDuringIntervalMap> interval_map =
        base::MakeUnique<TabsStateDuringIntervalMap>();
    ResetIntervalData(interval_map.get());

    // Setup the timer associated with this interval.
    std::unique_ptr<base::RepeatingTimer> timer =
        base::MakeUnique<base::RepeatingTimer>();
    timer->Start(FROM_HERE, base::TimeDelta::FromSeconds(kIntervalsInSec[i]),
                 Bind(&TabStatsTracker::OnInterval, base::Unretained(this),
                      interval_map.get(), kIntervalsInSec[i]));
    interval_maps_.push_back(std::move(interval_map));
    usage_interval_timers_.push_back(std::move(timer));
  }
}

TabStatsTracker::~TabStatsTracker() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
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
  DCHECK_EQ(nullptr, g_instance);
  g_instance = instance.release();
}

TabStatsTracker* TabStatsTracker::GetInstance() {
  return g_instance;
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
  existing_tabs_.insert(web_contents);

  tab_stats_data_store_->UpdateMaxTabsPerWindowIfNeeded(
      static_cast<size_t>(model->count()));
}

void TabStatsTracker::TabClosingAt(TabStripModel* model,
                                   content::WebContents* web_contents,
                                   int index) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(existing_tabs_.find(web_contents) != existing_tabs_.end());
  tab_stats_data_store_->OnTabsRemoved(1);
  existing_tabs_.erase(web_contents);
}

void TabStatsTracker::ActiveTabChanged(content::WebContents* old_contents,
                                       content::WebContents* new_contents,
                                       int index,
                                       int reason) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (old_contents != nullptr) {
    DCHECK(active_tabs_.find(old_contents) != active_tabs_.end());
    active_tabs_.erase(old_contents);
  }
  active_tabs_.insert(new_contents);

  for (auto& interval_map : interval_maps_) {
    auto iter = interval_map->find(new_contents);
    if (iter != interval_map->end()) {
      iter->second.interacted_during_interval = true;
      // TODO: Find a signal to check when the tab becomes visible.
      iter->second.visible_during_interval = true;
    }
  }
}

void TabStatsTracker::OnResume() {
  reporting_delegate_->ReportTabCountOnResume(
      tab_stats_data_store_->tab_stats().total_tab_count);
}

void TabStatsTracker::OnInterval(TabsStateDuringIntervalMap* interval_map,
                                 size_t interval_time_in_sec) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Counts the number of unused tabs during this interval, a tabs counts as
  // unused if it hasn't been interacted with or visible during the duration of
  // the interval.
  size_t unused_tabs = 0;
  for (const auto& iter : *interval_map) {
    bool exists_after_interval =
        existing_tabs_.find(iter.first) != existing_tabs_.end();
    if (exists_after_interval && !iter.second.visible_during_interval &&
        !iter.second.interacted_during_interval) {
      ++unused_tabs;
    }
  }

  std::string histogram_name = base::StringPrintf(
      "%s_%zu",
      UmaStatsReportingDelegate::kUnusedTabsInIntervalHistogramNameBase,
      interval_time_in_sec);

  // TODO: Forward this to the UmaStatsReportingDelegate.
  base::UmaHistogramCounts10000(histogram_name, unused_tabs);
  // Reset the interval data.
  ResetIntervalData(interval_map);
}

void TabStatsTracker::ResetIntervalData(
    TabsStateDuringIntervalMap* interval_map) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(interval_map != nullptr);
  interval_map->clear();
  for (const content::WebContents* web_contents : existing_tabs_) {
    bool interacted_during_interval =
        active_tabs_.find(web_contents) != active_tabs_.end();
    (*interval_map)[web_contents] = {
        true,                       // existed_before_interval
        true,                       // exists_after_interval
        web_contents->IsVisible(),  // visible_during_interval
        interacted_during_interval};
  }
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
    const TabStatsDataStore::TabsStats& tab_stats) {
  // Don't report the counts if they're equal to 0, this means that Chrome has
  // only been running in the background since the last time the metrics have
  // been reported.
  if (tab_stats.total_tab_count_max == 0)
    return;
  UMA_HISTOGRAM_COUNTS_10000(kMaxTabsInADayHistogramName,
                             tab_stats.total_tab_count_max);
  UMA_HISTOGRAM_COUNTS_10000(kMaxTabsPerWindowInADayHistogramName,
                             tab_stats.max_tab_per_window);
  UMA_HISTOGRAM_COUNTS_10000(kMaxWindowsInADayHistogramName,
                             tab_stats.window_count_max);
}

}  // namespace metrics
