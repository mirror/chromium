// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_TAB_STATS_TRACKER_H_
#define CHROME_BROWSER_METRICS_TAB_STATS_TRACKER_H_

#include "base/macros.h"
#include "base/power_monitor/power_observer.h"
#include "base/sequence_checker.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class PrefService;

namespace metrics {

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

  // The delegate that reports the events.
  std::unique_ptr<UmaStatsReportingDelegate> reporting_delegate_;

  // The tab stats.
  std::unique_ptr<TabsStatsDataStore> tab_stats_data_store_;

 private:
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(TabStatsTracker);
};

// The reporting delegate interface, which reports metrics via UMA.
class TabStatsTracker::UmaStatsReportingDelegate {
 public:
  // The name of the histogram that records the number of tabs total at resume
  // from sleep/hibernate.
  static const char kNumberOfTabsOnResumeHistogramName[];

  UmaStatsReportingDelegate() {}
  ~UmaStatsReportingDelegate() {}

  // Called at resume from sleep/hibernate.
  void ReportTabCountOnResume(size_t tab_count);

 private:
  DISALLOW_COPY_AND_ASSIGN(UmaStatsReportingDelegate);
};

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

  TabsStatsDataStore(PrefService* pref_service) : pref_service_(pref_service) {
  }
  ~TabsStatsDataStore() {
    ::memset(reinterpret_cast<uint8_t*>(&tab_stats_), 0, sizeof(TabStats));
  }

  void UpdateTotalTabCountMaxIfNeeded();
  void UpdateMaxTabsPerWindowIfNeeded(size_t value);
  void UpdateBrowserCountMaxIfNeeded();

  void UpdatePrefs();

  TabStats& tab_stats() { return tab_stats_; }

 protected:
  TabStats tab_stats_;

  // A weak pointer to the PrefService used to read and write the statistics.
  PrefService* pref_service_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TabsStatsDataStore);
};

}  // namespace metrics

#endif  // CHROME_BROWSER_METRICS_TAB_STATS_TRACKER_H_
