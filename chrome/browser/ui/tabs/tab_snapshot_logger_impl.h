// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TABS_TAB_SNAPSHOT_LOGGER_IMPL_H_
#define CHROME_BROWSER_UI_TABS_TAB_SNAPSHOT_LOGGER_IMPL_H_

#include "chrome/browser/ui/tabs/tab_snapshot_logger.h"

#include "base/macros.h"

// Logs a TabManager.TabSnapshot UKM for a tab when requested. Includes
// information relevant to the tab and its WebContents.
// Must be used on the UI thread.
// TODO(michaelpg): Unit test for UKMs.
class TabSnapshotLoggerImpl : public TabSnapshotLogger {
 public:
  TabSnapshotLoggerImpl();
  ~TabSnapshotLoggerImpl() override;

  // TabSnapshotLogger:
  void LogBackgroundTab(ukm::SourceId ukm_source_id,
                        const TabSnapshot& tab_snapshot) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TabSnapshotLoggerImpl);
};

#endif  // CHROME_BROWSER_UI_TABS_TAB_SNAPSHOT_LOGGER_IMPL_H_
