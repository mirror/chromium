// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_TAB_STATS_TRACKER_H_
#define CHROME_BROWSER_METRICS_TAB_STATS_TRACKER_H_

#include <set>

#include "base/macros.h"
#include "base/power_monitor/power_observer.h"
#include "base/sequence_checker.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

namespace metrics {

// Class for tracking and recording the tabs and browser windows usage.
//
// TODO(sebmarchand): This is just a base class and doesn't do much for now,
// finish this and document the architecture here.
class TabStatsTracker : public TabStripModelObserver,
                        public chrome::BrowserListObserver,
                        public base::PowerObserver {
 public:
  // Houses all of the statistics gathered by the TabStatsTracker.
  struct TabStats {
    // Constructor, initializes everything to zero.
    TabStats();

    // The total number of tabs opened across all the browsers.
    size_t total_tabs_count;

    // The maximum total number of tabs that has been opened across all the
    // browsers at the same time.
    size_t total_tabs_count_max;

    // The maximum total number of tabs that has been opened at the same time in
    // a single browser.
    size_t max_tabs_per_window;

    // The total number of browsers opened.
    size_t browser_count;

    // The maximum total number of browsers opened at the same time.
    size_t browser_count_max;
  };

  // The StatsReportingDelegate is responsible for delivering statistics
  // reported by the TabStatsTracker.
  class StatsReportingDelegate;

  // An implementation of StatsReportingDelegate for reporting via UMA.
  class UmaStatsReportingDelegate;

  // Creates the |TabStatsTracker| instance and initializes the
  // observers that notify it.
  static void Initialize();

  // Returns the |TabStatsTracker| instance.
  static TabStatsTracker* GetInstance();

  // Accessors.
  const TabStats& tab_stats() const { return tab_stats_; }

 protected:
  TabStatsTracker();
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

  // The tab stats.
  TabStats tab_stats_;

  // The delegate that reports the events.
  std::unique_ptr<StatsReportingDelegate> reporting_delegate_;

 private:
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(TabStatsTracker);
};

// The reporting delegate interface.
class TabStatsTracker::StatsReportingDelegate {
 public:
  StatsReportingDelegate() {}
  virtual ~StatsReportingDelegate() {}

  // Called at resume from sleep/hibernate.
  virtual void ReportTabCountOnResume(size_t tab_count) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(StatsReportingDelegate);
};

// The default reporting delegate, which reports via UMA.
class TabStatsTracker::UmaStatsReportingDelegate
    : public TabStatsTracker::StatsReportingDelegate {
 public:
  // The number of tabs total at resume from sleep/hibernate.
  static const char kNumberOfTabsOnResume[];

  UmaStatsReportingDelegate() {}
  ~UmaStatsReportingDelegate() override {}

  // Called at resume from sleep/hibernate.
  void ReportTabCountOnResume(size_t tab_count) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(UmaStatsReportingDelegate);
};

}  // namespace metrics

#endif  // CHROME_BROWSER_METRICS_TAB_STATS_TRACKER_H_
