// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/tab_stats_tracker.h"

#include <algorithm>
#include <string>
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
#include "chrome/common/pref_names.h"
#include "components/metrics/daily_event.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace metrics {

namespace {

// The interval at which the DailyEvent::CheckInterval function should be
// called.
constexpr base::TimeDelta kDailyEventIntervalTimeDelta =
    base::TimeDelta::FromMilliseconds(60 * 30);

// The global TabStatsTracker instance.
TabStatsTracker* g_instance = nullptr;

// The intervals at which we report the number of unused tabs.
constexpr base::TimeDelta kTabUsageReportingIntervals[] = {
    base::TimeDelta::FromSeconds(30), base::TimeDelta::FromMinutes(1),
    base::TimeDelta::FromMinutes(10), base::TimeDelta::FromHours(1),
    base::TimeDelta::FromHours(5),    base::TimeDelta::FromHours(12)};

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
    kUnusedAndClosedInIntervalHistogramNameBase[] =
        "Tabs.UnusedAndClosedInInterval.Count";
const char TabStatsTracker::UmaStatsReportingDelegate::
    kUnusedTabsInIntervalHistogramNameBase[] = "Tabs.UnusedInInterval.Count";
const char TabStatsTracker::UmaStatsReportingDelegate::
    kUsedAndClosedInIntervalHistogramNameBase[] =
        "Tabs.UsedAndClosedInInterval.Count";
const char TabStatsTracker::UmaStatsReportingDelegate::
    kUsedTabsInIntervalHistogramNameBase[] = "Tabs.UsedInInterval.Count";

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
    OnBrowserAdded(browser);
    for (int i = 0; i < browser->tab_strip_model()->count(); ++i) {
      OnTabAdded(browser->tab_strip_model()->GetWebContentsAt(i));
    }
    tab_stats_data_store_->UpdateMaxTabsPerWindowIfNeeded(
        static_cast<size_t>(browser->tab_strip_model()->count()));
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

  // Initialize the interval maps and timers associated with them.
  for (size_t i = 0; i < arraysize(kTabUsageReportingIntervals); ++i) {
    tab_stats_data_store_->AddInterval(
        kTabUsageReportingIntervals[i].InSeconds());
    // Setup the timer associated with this interval.
    std::unique_ptr<base::RepeatingTimer> timer =
        base::MakeUnique<base::RepeatingTimer>();
    timer->Start(FROM_HERE, kTabUsageReportingIntervals[i],
                 Bind(&TabStatsTracker::OnInterval, base::Unretained(this),
                      kTabUsageReportingIntervals[i].InSeconds()));
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

void TabStatsTracker::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(prefs::kTabStatsTotalTabCountMax, 0);
  registry->RegisterIntegerPref(prefs::kTabStatsMaxTabsPerWindow, 0);
  registry->RegisterIntegerPref(prefs::kTabStatsWindowCountMax, 0);
  DailyEvent::RegisterPref(registry, prefs::kTabStatsDailySample);
}

void TabStatsTracker::TabStatsDailyObserver::OnDailyEvent(
    DailyEvent::IntervalType type) {
  reporting_delegate_->ReportDailyMetrics(data_store_->tab_stats());
  data_store_->ResetMaximumsToCurrentState();
}

void TabStatsTracker::WebContentsUsageObserver::DidGetUserInteraction(
    const blink::WebInputEvent::Type type) {
  data_store_->OnTabInteraction(web_contents());
}

void TabStatsTracker::WebContentsUsageObserver::WasShown() {
  data_store_->OnTabVisible(web_contents());
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
  OnTabAdded(web_contents);

  tab_stats_data_store_->UpdateMaxTabsPerWindowIfNeeded(
      static_cast<size_t>(model->count()));
}

void TabStatsTracker::TabClosingAt(TabStripModel* model,
                                   content::WebContents* web_contents,
                                   int index) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  tab_stats_data_store_->OnTabRemoved(web_contents);
}

void TabStatsTracker::TabChangedAt(content::WebContents* web_contents,
                                   int index,
                                   TabChangeType change_type) {
  if (change_type != TabChangeType::kAll)
    return;
  tab_stats_data_store_->OnTabChangedEvent(web_contents);
}

void TabStatsTracker::OnResume() {
  reporting_delegate_->ReportTabCountOnResume(
      tab_stats_data_store_->tab_stats().total_tab_count);
}

void TabStatsTracker::OnInterval(size_t interval_time_in_sec) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  TabStatsDataStore::TabsStateDuringIntervalMap* interval_map =
      tab_stats_data_store_->GetIntervalMap(interval_time_in_sec);
  CHECK_NE(nullptr, interval_map);
  reporting_delegate_->ReportUsageDuringInterval(*interval_map,
                                                 interval_time_in_sec);
  // Reset the interval data.
  tab_stats_data_store_->ResetIntervalData(interval_map);
}

void TabStatsTracker::OnTabAdded(content::WebContents* web_contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  tab_stats_data_store_->OnTabAdded(web_contents);
  web_contents_usage_observers_.insert(std::make_pair(
      web_contents, base::MakeUnique<WebContentsUsageObserver>(
                        web_contents, tab_stats_data_store_.get())));
}

void TabStatsTracker::OnTabRemoved(content::WebContents* web_contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  tab_stats_data_store_->OnTabRemoved(web_contents);
  DCHECK(web_contents_usage_observers_.find(web_contents) !=
         web_contents_usage_observers_.end());
  web_contents_usage_observers_.erase(
      web_contents_usage_observers_.find(web_contents));
}

void TabStatsTracker::UmaStatsReportingDelegate::ReportTabCountOnResume(
    size_t tab_count) {
  // Don't report the number of tabs on resume if Chrome is running in
  // background with no visible window.
  if (IsChromeBackgroundedWithoutWindows())
    return;
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

void TabStatsTracker::UmaStatsReportingDelegate::ReportUsageDuringInterval(
    const TabStatsDataStore::TabsStateDuringIntervalMap& interval_map,
    size_t interval_time_in_sec) {
  // Counts the number of used/unused tabs during this interval, a tabs counts
  // as unused if it hasn't been interacted with or visible during the duration
  // of the interval.
  size_t used_tabs = 0;
  size_t used_and_closed_tabs = 0;
  size_t unused_tabs = 0;
  size_t unused_and_closed_tabs = 0;
  for (auto iter : interval_map) {
    if (iter.second.interacted_during_interval ||
        iter.second.visible_or_audible_during_interval) {
      if (iter.second.exists_after_interval)
        ++used_tabs;
      else
        ++used_and_closed_tabs;
    } else {
      if (iter.second.exists_after_interval)
        ++unused_tabs;
      else
        ++unused_and_closed_tabs;
    }
  }

  std::string used_and_closed_histogram_name = base::StringPrintf(
      "%s_%zu",
      UmaStatsReportingDelegate::kUsedAndClosedInIntervalHistogramNameBase,
      interval_time_in_sec);
  std::string used_histogram_name = base::StringPrintf(
      "%s_%zu", UmaStatsReportingDelegate::kUsedTabsInIntervalHistogramNameBase,
      interval_time_in_sec);
  std::string unused_and_closed_histogram_name = base::StringPrintf(
      "%s_%zu",
      UmaStatsReportingDelegate::kUnusedAndClosedInIntervalHistogramNameBase,
      interval_time_in_sec);
  std::string unused_histogram_name = base::StringPrintf(
      "%s_%zu",
      UmaStatsReportingDelegate::kUnusedTabsInIntervalHistogramNameBase,
      interval_time_in_sec);

  base::UmaHistogramCounts10000(used_and_closed_histogram_name,
                                used_and_closed_tabs);
  base::UmaHistogramCounts10000(used_histogram_name, used_tabs);
  base::UmaHistogramCounts10000(unused_and_closed_histogram_name,
                                unused_and_closed_tabs);
  base::UmaHistogramCounts10000(unused_histogram_name, unused_tabs);
}

bool TabStatsTracker::UmaStatsReportingDelegate::
    IsChromeBackgroundedWithoutWindows() {
  if (g_browser_process && g_browser_process->background_mode_manager()
                               ->IsBackgroundWithoutWindows()) {
    return true;
  }
  return false;
}

}  // namespace metrics
