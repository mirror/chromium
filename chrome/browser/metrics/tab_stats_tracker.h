// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_TAB_STATS_TRACKER_H_
#define CHROME_BROWSER_METRICS_TAB_STATS_TRACKER_H_

#include "base/lazy_instance.h"
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
  // The StatsReportingDelegate is responsible for delivering statistics
  // reported by the TabStatsTracker.
  class StatsReportingDelegate;

  // An implementation of StatsReportingDelegate for reporting via UMA.
  class UmaStatsReportingDelegate;

  // Creates the |TabStatsTracker| instance and initializes the
  // observers that notify it.
  static void Initialize(std::unique_ptr<StatsReportingDelegate> reporting_delegate);

  // Returns the |TabStatsTracker| instance.
  static TabStatsTracker* GetInstance();

  // Accessors.
  size_t total_tab_count() const { return total_tabs_count_; }
  size_t browser_count() const { return browser_count_; }

 protected:
  friend struct base::LazyInstanceTraitsBase<TabStatsTracker>;

  TabStatsTracker();
  ~TabStatsTracker() override;

  void set_reporting_delegate(std::unique_ptr<StatsReportingDelegate> reporting_delegate) {
    reporting_delegate_ = std::move(reporting_delegate);
  }

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

  // The total number of tabs opened across all the browsers.
  size_t total_tabs_count_;

  // The total number of browsers opened.
  size_t browser_count_;

  std::unique_ptr<StatsReportingDelegate> reporting_delegate_;

 private:
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(TabStatsTracker);
};

// An abstract reporting delegate is used as a testing seam.
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
  UmaStatsReportingDelegate() {}
  ~UmaStatsReportingDelegate() override {}

  // Called at resume from sleep/hibernate.
  void ReportTabCountOnResume(size_t tab_count) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(UmaStatsReportingDelegate);
};

}  // namespace metrics

#endif  // CHROME_BROWSER_METRICS_TAB_STATS_TRACKER_H_
