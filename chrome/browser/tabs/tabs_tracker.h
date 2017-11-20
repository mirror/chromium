// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TABS_TABS_TRACKER_H_
#define CHROME_BROWSER_TABS_TABS_TRACKER_H_

#include <memory>
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class BrowserTabStripTracker;

namespace tabs {

class TabLogger;

class TabsTracker : public TabStripModelObserver {
 public:
  TabsTracker();
  explicit TabsTracker(std::unique_ptr<tabs::TabLogger> tab_logger);
  ~TabsTracker() override;

  // TabStripModelObserver:
  void TabDeactivated(content::WebContents* contents) override;
  void ActiveTabChanged(content::WebContents* old_contents,
                        content::WebContents* new_contents,
                        int index,
                        int reason) override;
  void TabPinnedStateChanged(TabStripModel* tab_strip_model,
                             content::WebContents* contents,
                             int index) override;
  // TODO(michaelpg): Track more events.

 private:
  std::unique_ptr<tabs::TabLogger> tab_logger_;

  // Manages registration of this class as an observer of all TabStripModels as
  // browsers are created and destroyed.
  std::unique_ptr<BrowserTabStripTracker> browser_tab_strip_tracker_;
};

}  // namespace tabs

#endif  // CHROME_BROWSER_TABS_TABS_TRACKER_H_
