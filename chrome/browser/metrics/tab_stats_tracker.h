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

class DailyEvent;

// Class for tracking and recording the tabs and browser windows usage.
//
// TODO(sebmarchand): This is just a base class and doesn't do much for now,
// finish this and document the architecture here.
class TabStatsTracker : public TabStripModelObserver,
                        public chrome::BrowserListObserver,
                        public base::PowerObserver {
 public:
  class TabsStatsDataStore;

  // Creates the |TabStatsTracker| instance and initializes the observers that
  // notify it.
  static void Initialize(PrefService* pref_service);

  // Returns the |TabStatsTracker| instance.
  static TabStatsTracker* GetInstance();

  // Accessors.
  TabsStatsDataStore* tab_stats_data_store() {
    return tab_stats_data_store_.get();
  }

 protected:
  // The UmaStatsReportingDelegate is responsible for delivering statistics
  // reported by the TabStatsTracker via UMA.
  class UmaStatsReportingDelegate;

  class TabStatsDailyObserver : public DailyEvent::Observer {
   public:
    TabStatsDailyObserver(UmaStatsReportingDelegate* reporting_delegate,
                          TabsStatsDataStore* data_store)
        : reporting_delegate_(reporting_delegate), data_store_(data_store) {}
    ~TabStatsDailyObserver() {}

    void OnDailyEvent() override;

   private:
    UmaStatsReportingDelegate* reporting_delegate_;
    TabsStatsDataStore* data_store_;
    DISALLOW_COPY_AND_ASSIGN(TabStatsDailyObserver);
  };

  TabStatsTracker(PrefService* pref_service);
  ~TabStatsTracker() override;

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

  static const char kTabStatsDailyEventHistogramName[];

  // The delegate that reports the events.
  std::unique_ptr<UmaStatsReportingDelegate> reporting_delegate_;

  // The tab stats.
  std::unique_ptr<TabsStatsDataStore> tab_stats_data_store_;

  // A daily event for collecting metrics once a day.
  std::unique_ptr<DailyEvent> daily_event_;

  base::RepeatingTimer timer_;

 private:
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(TabStatsTracker);
};

// The data store that keeps track of all the information that the
// TabStatsTracker want to track.
class TabStatsTracker::TabsStatsDataStore {
 public:
  // Houses all of the statistics gathered by the TabStatsTracker.
  struct TabStats {
    // Constructor, initializes everything to zero.
    TabStats();

    // The total number of tabs opened across all the browsers.
    size_t total_tab_count;

    // The maximum total number of tabs that has been opened across all the
    // browsers at the same time.
    size_t total_tab_count_max;

    // The maximum total number of tabs that has been opened at the same time in
    // a single browser.
    size_t max_tab_per_window;

    // The total number of browsers opened.
    size_t browser_count;

    // The maximum total number of browsers opened at the same time.
    size_t browser_count_max;
  };

  TabsStatsDataStore(PrefService* pref_service);
  ~TabsStatsDataStore() {}

  void OnBrowsersAdded(size_t browser_count);
  void OnBrowsersRemoved(size_t browser_count);

  void OnTabsAdded(size_t tab_count);
  void OnTabsRemoved(size_t tab_count);

  void UpdateMaxTabsPerWindowIfNeeded(size_t value);

  void ResetMaximumsToCurrentState();

  const TabStats& tab_stats() const { return tab_stats_; }

 protected:
  void UpdateTotalTabCountMaxIfNeeded();
  void UpdateBrowserCountMaxIfNeeded();

  TabStats tab_stats_;

  // A weak pointer to the PrefService used to read and write the statistics.
  PrefService* pref_service_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TabsStatsDataStore);
};

// The reporting delegate interface, which reports metrics via UMA.
class TabStatsTracker::UmaStatsReportingDelegate {
 public:
  // The name of the histogram that records the number of tabs total at resume
  // from sleep/hibernate.
  static const char kNumberOfTabsOnResumeHistogramName[];
  static const char kMaxTabsInADayHistogramName[];
  static const char kMaxTabsPerBrowserInADayHistogramName[];
  static const char kMaxBrowsersInADayHistogramName[];

  UmaStatsReportingDelegate() {}
  ~UmaStatsReportingDelegate() {}

  // Called at resume from sleep/hibernate.
  void ReportTabCountOnResume(size_t tab_count);

  void ReportDailyMetrics(
      const TabStatsTracker::TabsStatsDataStore::TabStats& tab_stats);

 private:
  DISALLOW_COPY_AND_ASSIGN(UmaStatsReportingDelegate);
};

}  // namespace metrics

#endif  // CHROME_BROWSER_METRICS_TAB_STATS_TRACKER_H_
