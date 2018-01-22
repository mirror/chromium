// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TABS_WINDOW_ACTIVITY_WATCHER_H_
#define CHROME_BROWSER_UI_TABS_WINDOW_ACTIVITY_WATCHER_H_

#include <stddef.h>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/browser_tab_strip_tracker.h"
#include "chrome/browser/ui/browser_tab_strip_tracker_delegate.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/sessions/core/session_id.h"

// Observes browser window activity in order to log WindowMetrics UKMs for
// browser events relative to tab activation and discarding.
// Multiple tabs in the same browser can refer to the same WindowMetrics entry.
// Must be used on the UI thread.
// TODO(michaelpg): Observe app and ARC++ windows as well.
class WindowActivityWatcher : public BrowserListObserver,
                              public TabStripModelObserver,
                              public BrowserTabStripTrackerDelegate {
 public:
  // Represents a UKM entry for window metrics.
  struct WindowMetrics;

  // Returns the single instance, creating it if necessary.
  static WindowActivityWatcher* GetInstance();

  // Ensures the window's current stats are logged.
  // A new UKM entry will only be logged if an existing entry for the same
  // window doesn't exist yet or has stale properties.
  void CreateOrUpdateWindowMetrics(const Browser* browser);

 private:
  WindowActivityWatcher();
  ~WindowActivityWatcher() override;

  // BrowserListObserver:
  void OnBrowserAdded(Browser* browser) override;
  void OnBrowserRemoved(Browser* browser) override;
  void OnBrowserSetLastActive(Browser* browser) override;
  void OnBrowserNoLongerActive(Browser* browser) override;

  // TabStripModelObserver:
  void TabInsertedAt(TabStripModel* tab_strip_model,
                     content::WebContents* contents,
                     int index,
                     bool foreground) override;
  void TabDetachedAt(content::WebContents* contents, int index) override;
  void TabReplacedAt(TabStripModel* tab_strip_model,
                     content::WebContents* old_contents,
                     content::WebContents* new_contents,
                     int index) override;

  // BrowserTabStripTrackerDelegate:
  bool ShouldTrackBrowser(Browser* browser) override;

  // Used to update the tab count for browser windows.
  BrowserTabStripTracker browser_tab_strip_tracker_;

  // WindowMetrics for each loggable browser window.
  base::flat_map<SessionID::id_type, WindowMetrics> window_metrics_;

  // Maps WebContents to the ID of their most recent containing browser. This
  // allows updating the WindowMetrics entry for a browser after one of its tabs
  // is removed (if the browser is still alive).
  base::flat_map<content::WebContents*, SessionID::id_type>
      web_contents_to_browser_id_;

  DISALLOW_COPY_AND_ASSIGN(WindowActivityWatcher);
};

#endif  // CHROME_BROWSER_UI_TABS_WINDOW_ACTIVITY_WATCHER_H_
