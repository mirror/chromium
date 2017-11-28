// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TABS_TAB_SNAPSHOT_LOGGER_H_
#define CHROME_BROWSER_UI_TABS_TAB_SNAPSHOT_LOGGER_H_

#include <map>

#include "base/macros.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

namespace content {
class WebContents;
}

// Logs TabManager.Tab UKM for a tab when requested. Includes information
// relevant to the tab and its WebContents.
// Must be used on the UI thread.
// TODO(michaelpg): Unit test for UKMs.
class TabSnapshotLogger {
 public:
  TabSnapshotLogger();
  virtual ~TabSnapshotLogger();

  // Logs stats for the tab with the given main frame WebContents. Does nothing
  // if |ukm_source_id| is zero.
  // Virtual for testing.
  virtual void LogBackgroundTab(ukm::SourceId ukm_source_id,
                                content::WebContents* web_contents);

 private:
  DISALLOW_COPY_AND_ASSIGN(TabSnapshotLogger);
};

#endif  // CHROME_BROWSER_UI_TABS_TAB_SNAPSHOT_LOGGER_H_
