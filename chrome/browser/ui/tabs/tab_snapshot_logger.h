// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TABS_TAB_SNAPSHOT_LOGGER_H_
#define CHROME_BROWSER_UI_TABS_TAB_SNAPSHOT_LOGGER_H_

#include "base/macros.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

namespace content {
class WebContents;
}

// Logs stats for a tab and its WebContents when requested.
// Must be used on the UI thread.
class TabSnapshotLogger {
 public:
  // The state of the page loaded in a tab's main frame.
  struct PageSnapshot {
    // Number of key events.
    int key_event_count;
    // Number of mouse events.
    int mouse_event_count;
    // Number of touch events.
    int touch_event_count;
  };

  // The state of a tab.
  struct TabSnapshot {
    content::WebContents* web_contents;

    // Per-page snapshot of the state of the WebContents' current page. Tracked
    // since the last the tab's last top-level navigation.
    PageSnapshot page_snapshot;
  };

  TabSnapshotLogger() = default;
  virtual ~TabSnapshotLogger() = default;

  // Logs stats for the tab with the given main frame WebContents. Does nothing
  // if |ukm_source_id| is zero.
  virtual void LogBackgroundTab(ukm::SourceId ukm_source_id,
                                const TabSnapshot& tab_snapshot) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(TabSnapshotLogger);
};

#endif  // CHROME_BROWSER_UI_TABS_TAB_SNAPSHOT_LOGGER_H_
