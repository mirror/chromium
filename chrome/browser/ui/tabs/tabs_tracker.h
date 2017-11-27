// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TABS_TABS_TRACKER_H_
#define CHROME_BROWSER_UI_TABS_TABS_TRACKER_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/ui/browser_tab_strip_tracker.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class TabLogger;

// Observes background tabs in order to log UKMs. Metrics will be compared
// against tab reactivation/close events to determine the end state of each
// background tab.
class TabsTracker : public TabStripModelObserver {
 public:
  // Helper class to observe WebContents.
  class WebContentsData;

  TabsTracker();
  ~TabsTracker() override;

  // TabStripModelObserver:
  void TabPinnedStateChanged(TabStripModel* tab_strip_model,
                             content::WebContents* contents,
                             int index) override;
  // TODO(michaelpg): Track more events.

  void SetTabLoggerForTest(std::unique_ptr<TabLogger> tab_logger);

  // Returns the single instance, creating it if necessary.
  static TabsTracker* GetInstance();

  // Creates a TabTracker::WebContentsData to observe the |web_contents|.
  static void InitializeWebContentsDataForWebContents(
      content::WebContents* web_contents);

 private:
  // Called from WebContentsData.
  void OnWasHidden(content::WebContents* web_contents);

  std::unique_ptr<TabLogger> tab_logger_;

  // Manages registration of this class as an observer of all TabStripModels as
  // browsers are created and destroyed.
  BrowserTabStripTracker browser_tab_strip_tracker_;

  DISALLOW_COPY_AND_ASSIGN(TabsTracker);
};

#endif  // CHROME_BROWSER_UI_TABS_TABS_TRACKER_H_
