// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TABS_TAB_LOGGER_H_
#define CHROME_BROWSER_TABS_TAB_LOGGER_H_

#include <stdint.h>

#include <map>

#include "base/time/time.h"
// TODO(michaelpg): This circular include is necessary because WebContentsData
// is an inner class of TabManager. Ideally it wouldn't be.
#include "chrome/browser/resource_coordinator/tab_manager_web_contents_data.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

class TabStripModel;

namespace tabs {

// Logs UKMs for tabs on navigation, tab activation and tab close events.
class TabLogger {
 public:
  using WebContentsId = int64_t;
  TabLogger();
  ~TabLogger();

  // Logs stats for the tab with the WebContentsData and tab strip location.
  void LogTab(const resource_coordinator::TabManager::WebContentsData*
                  web_contents_data,
              const TabStripModel* tab_strip_model,
              int tab_index);

 private:
  std::map<ukm::SourceId, base::TimeTicks> last_log_times_;
};

}  // namespace tabs

#endif  // CHROME_BROWSER_TABS_TAB_LOGGER_H_
