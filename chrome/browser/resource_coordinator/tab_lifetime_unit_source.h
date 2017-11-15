// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_LIFETIME_UNIT_SOURCE_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_LIFETIME_UNIT_SOURCE_H_

#include <memory>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/browser_tab_strip_tracker.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class TabStripModel;

namespace content {
class WebContents;
}

namespace resource_coordinator {

class LifetimeUnitSink;
class TabLifetimeObserver;
class TabLifetimeUnitImpl;

// Creates / destroys TabLifetimeUnits and notifies a LifetimeUnitSink as tabs
// are created / destroyed.
class TabLifetimeUnitSource : public TabStripModelObserver,
                              public chrome::BrowserListObserver {
 public:
  TabLifetimeUnitSource(LifetimeUnitSink* sink);
  ~TabLifetimeUnitSource() override;

  // Adds / removes an observer that is notified of changes to the lifetime of
  // tabs.
  void AddObserver(TabLifetimeObserver* observer);
  void RemoveObserver(TabLifetimeObserver* observer);

  // Pretend that |tab_strip| is in a focused browser window.
  void SetFocusedTabStripModelForTesting(TabStripModel* tab_strip);

 private:
  void UpdateFocusedTab();

  // TabStripModelObserver:
  void TabInsertedAt(TabStripModel* tab_strip_model,
                     content::WebContents* contents,
                     int index,
                     bool foreground) override;
  void TabClosingAt(TabStripModel* tab_strip_model,
                    content::WebContents* contents,
                    int index) override;
  void ActiveTabChanged(content::WebContents* old_contents,
                        content::WebContents* new_contents,
                        int index,
                        int reason) override;
  void TabReplacedAt(TabStripModel* tab_strip_model,
                     content::WebContents* old_contents,
                     content::WebContents* new_contents,
                     int index) override;
  void TabChangedAt(content::WebContents* contents,
                    int index,
                    TabChangeType change_type) override;

  // chrome::BrowserListObserver:
  void OnBrowserSetLastActive(Browser* browser) override;
  void OnBrowserNoLongerActive(Browser* browser) override;

  BrowserTabStripTracker browser_tab_strip_tracker_;

  LifetimeUnitSink* sink_;

  TabStripModel* focused_tab_strip_model_for_testing_ = nullptr;

  TabLifetimeUnitImpl* focused_tab_lifetime_unit_ = nullptr;

  base::flat_map<content::WebContents*, std::unique_ptr<TabLifetimeUnitImpl>>
      tabs_;

  base::ObserverList<TabLifetimeObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(TabLifetimeUnitSource);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_LIFETIME_UNIT_SOURCE_H_
