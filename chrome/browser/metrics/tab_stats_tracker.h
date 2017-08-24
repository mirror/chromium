// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_TAB_STATS_TRACKER_H_
#define CHROME_BROWSER_METRICS_TAB_STATS_TRACKER_H_

#include "base/macros.h"
#include "base/power_monitor/power_observer.h"
#include "base/sequence_checker.h"
#include "base/timer/timer.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/metrics/daily_event.h"

class PrefService;

namespace metrics {

// The data store that keeps track of all the information that the
// TabStatsTracker wants to track.
class TabsStatsDataStore {
 public:
  // Houses all of the statistics gathered by the TabStatsTracker.
  struct TabsStats {
    // Constructor, initializes everything to zero.
    TabsStats();

    // The total number of tabs opened across all the windows.
    size_t total_tab_count;

    // The maximum total number of tabs that has been opened across all the
    // windows at the same time.
    size_t total_tab_count_max;

    // The maximum total number of tabs that has been opened at the same time in
    // a single window.
    size_t max_tab_per_window;

    // The total number of windows opened.
    size_t window_count;

    // The maximum total number of windows opened at the same time.
    size_t window_count_max;
  };

  explicit TabsStatsDataStore(PrefService* pref_service);
  ~TabsStatsDataStore() {}

  // Functions used to update the window/tab count.
  void OnWindowAdded();
  void OnWindowRemoved();
  void OnTabsAdded(size_t tab_count);
  void OnTabsRemoved(size_t tab_count);

  // Update the maximum number of tabs in a single window if |value| exceeds
  // this.
  // TODO(sebmarchand): Store a list of windows in this class and track the
  // number of tabs per window.
  void UpdateMaxTabsPerWindowIfNeeded(size_t value);

  // Reset all the maximum values to the current state, to be used once the
  // metrics have been reported.
  void ResetMaximumsToCurrentState();

  const TabsStats& tab_stats() const { return tab_stats_; }

 protected:
  // Update the maximums metrics if needed.
  void UpdateTotalTabCountMaxIfNeeded();
  void UpdateWindowCountMaxIfNeeded();

  // The tabs stats.
  TabsStats tab_stats_;

  // A weak pointer to the PrefService used to read and write the statistics.
  PrefService* pref_service_;

 private:
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(TabsStatsDataStore);
};

// Class for tracking and recording the tabs and browser windows usage.
//
// This class is meant to be used as a singleton by calling the SetInstance
// method, e.g.:
//     TabStatsTracker::SetInstance(
//         std::make_unique<TabStatsTracker>(g_browser_process->local_state()));
class TabStatsTracker : public TabStripModelObserver,
                        public chrome::BrowserListObserver,
                        public base::PowerObserver {
 public:
  // Sets the |TabStatsTracker| global instance.
  static void SetInstance(std::unique_ptr<TabStatsTracker> instance);

  // Returns the |TabStatsTracker| global instance.
  static TabStatsTracker* GetInstance();

  explicit TabStatsTracker(PrefService* pref_service);
  ~TabStatsTracker() override;

  // Accessors.
  const TabsStatsDataStore::TabsStats& tab_stats() const;

 protected:
  // The UmaStatsReportingDelegate is responsible for delivering statistics
  // reported by the TabStatsTracker via UMA.
  class UmaStatsReportingDelegate;

  // The observer that's used by |daily_event_| to report the metrics.
  class TabStatsDailyObserver : public DailyEvent::Observer {
   public:
    TabStatsDailyObserver(UmaStatsReportingDelegate* reporting_delegate,
                          TabsStatsDataStore* data_store)
        : reporting_delegate_(reporting_delegate), data_store_(data_store) {}
    ~TabStatsDailyObserver() override {}

    // Callback called when the daily event happen.
    void OnDailyEvent() override;

   private:
    // A weak pointer to the reporting delegate used to report the metrics.
    UmaStatsReportingDelegate* reporting_delegate_;

    // A weak pointer to the data store that houses the metrics.
    TabsStatsDataStore* data_store_;

    DISALLOW_COPY_AND_ASSIGN(TabStatsDailyObserver);
  };

  // chrome::BrowserListObserver:
  void OnBrowserAdded(Browser* browser) override;
  void OnBrowserRemoved(Browser* browser) override;

  // TabStripModelObserver:
  void TabInsertedAt(TabStripModel* tab_strip_model,
                     content::WebContents* contents,
                     int index,
                     bool foreground) override;
  void TabClosingAt(TabStripModel* model,
                    content::WebContents* web_contents,
                    int index) override;

  // base::PowerObserver:
  void OnResume() override;

  // The name of the histogram used to report that the daily event happened.
  static const char kTabStatsDailyEventHistogramName[];

  // The delegate that reports the events.
  std::unique_ptr<UmaStatsReportingDelegate> reporting_delegate_;

  // The tab stats.
  std::unique_ptr<TabsStatsDataStore> tab_stats_data_store_;

  // A daily event for collecting metrics once a day.
  std::unique_ptr<DailyEvent> daily_event_;

  // The timer used to periodically check if the daily event should be
  // triggered.
  base::RepeatingTimer timer_;

 private:
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(TabStatsTracker);
};

// The reporting delegate, which reports metrics via UMA.
class TabStatsTracker::UmaStatsReportingDelegate {
 public:
  // The name of the histogram that records the number of tabs total at resume
  // from sleep/hibernate.
  static const char kNumberOfTabsOnResumeHistogramName[];

  // The name of the histogram that records the maximum number of tabs opened in
  // a day.
  static const char kMaxTabsInADayHistogramName[];

  // The name of the histogram that records the maximum number of tabs opened in
  // the same window in a day.
  static const char kMaxTabsPerWindowInADayHistogramName[];

  // The name of the histogram that records the maximum number of windows
  // opened in a day.
  static const char kMaxWindowsInADayHistogramName[];

  UmaStatsReportingDelegate() {}
  ~UmaStatsReportingDelegate() {}

  // Called at resume from sleep/hibernate.
  void ReportTabCountOnResume(size_t tab_count);

  // Called once per day to report the metrics.
  void ReportDailyMetrics(const TabsStatsDataStore::TabsStats& tab_stats);

 private:
  DISALLOW_COPY_AND_ASSIGN(UmaStatsReportingDelegate);
};

}  // namespace metrics

#endif  // CHROME_BROWSER_METRICS_TAB_STATS_TRACKER_H_
