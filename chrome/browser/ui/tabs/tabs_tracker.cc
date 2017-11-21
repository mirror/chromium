// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tabs_tracker.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_tab_strip_tracker.h"
#include "chrome/browser/ui/tabs/tab_logger.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(tabs::TabsTracker::WebContentsData);

namespace tabs {

TabsTracker::WebContentsData::~WebContentsData() = default;

void TabsTracker::WebContentsData::WasHidden() {
  g_browser_process->GetTabsTracker()->OnWasHidden(web_contents());
}

TabsTracker::WebContentsData::WebContentsData(content::WebContents* contents)
    : WebContentsObserver(contents) {}

TabsTracker::TabsTracker()
    : tab_logger_(std::make_unique<tabs::TabLogger>()),
      browser_tab_strip_tracker_(
          std::make_unique<BrowserTabStripTracker>(this, nullptr, nullptr)) {
  browser_tab_strip_tracker_->Init();
}

void TabsTracker::SetTabLoggerForTest(
    std::unique_ptr<tabs::TabLogger> tab_logger) {
  this->tab_logger_ = std::move(tab_logger);
}

TabsTracker::~TabsTracker() = default;

void TabsTracker::OnWasHidden(content::WebContents* contents) {
  DCHECK(contents);

  // Log the state of the hidden tab.
  Browser* browser = chrome::FindBrowserWithWebContents(contents);
  if (!browser)
    return;

  TabStripModel* model = browser->tab_strip_model();
  int old_index = model->GetIndexOfWebContents(contents);
  DCHECK_NE(old_index, TabStripModel::kNoTab);
  tab_logger_->LogBackgroundTab(contents, browser, model, old_index);
}

void TabsTracker::TabPinnedStateChanged(TabStripModel* tab_strip_model,
                                        content::WebContents* contents,
                                        int index) {
  // We only log background tabs.
  if (tab_strip_model->active_index() == index)
    return;

  Browser* browser = chrome::FindBrowserWithWebContents(contents);
  DCHECK(browser);

  tab_logger_->LogBackgroundTab(contents, browser, tab_strip_model, index);
}

}  // namespace tabs
