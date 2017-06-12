// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_TAB_STATS_TAB_STATS_TRACKER_H_
#define CHROME_BROWSER_METRICS_TAB_STATS_TAB_STATS_TRACKER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/power_monitor/power_observer.h"
#include "chrome/browser/sessions/session_restore.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/metrics/daily_event.h"

namespace metrics {

class TabStatsTracker : public TabStripModelObserver,
                        public chrome::BrowserListObserver,
                        public base::PowerObserver,
                        public metrics::DailyEvent::Observer {
 public:
  // Creates the |TabStatsTracker| instance and initializes the
  // observers that notify to it.
  static void Initialize();

  // Returns true if the |TabStatsTracker| instance has been
  // created.
  static bool IsInitialized();

  // Returns the |TabStatsTracker| instance.
  static TabStatsTracker* Get();

 protected:
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


  // PowerObserver
  void OnResume() override;

  void OnSessionRestoreDone(int num_tabs);

  TabStatsTracker();
  ~TabStatsTracker();

  SessionRestore::CallbackSubscription
      on_session_restored_callback_subscription_;

  size_t total_tabs_count_;

  DISALLOW_COPY_AND_ASSIGN(TabStatsTracker);
};

}  // namespace metrics

#endif  // CHROME_BROWSER_METRICS_TAB_STATS_TAB_STATS_TRACKER_H_
