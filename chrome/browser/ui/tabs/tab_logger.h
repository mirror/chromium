// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TABS_TAB_LOGGER_H_
#define CHROME_BROWSER_UI_TABS_TAB_LOGGER_H_

#include <map>

#include "services/metrics/public/cpp/ukm_source_id.h"

class Browser;
class TabStripModel;

namespace base {
class TimeTicks;
}

namespace content {
class WebContents;
}  // namespace content

namespace tabs {

// Logs UKMs for tabs on navigation, tab activation and tab close events.
// Must be used on the UI thread.
// TODO(michaelpg): Unit test for UKMs.
class TabLogger {
 public:
  TabLogger();
  virtual ~TabLogger();

  // Logs stats for the tab with the WebContentsData and tab strip location.
  virtual void LogBackgroundTab(const content::WebContents* web_contents,
                                const Browser* browser,
                                const TabStripModel* tab_strip_model,
                                int index);

 private:
  // Creates and records the TabEntry.
  void RecordTabEntry(ukm::SourceId ukm_source_id,
                      const content::WebContents* contents,
                      const TabStripModel* tab_strip_model,
                      int index);

  // Used to throttle event logging per SourceId.
  std::map<ukm::SourceId, base::TimeTicks> last_log_times_;
};

}  // namespace tabs

#endif  // CHROME_BROWSER_UI_TABS_TAB_LOGGER_H_
