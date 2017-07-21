// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/tab_stats_tracker.h"

#include "base/power_monitor/power_monitor.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

namespace metrics {

namespace {

TabStatsTracker* g_instance = nullptr;

}  // namespace

// static
void TabStatsTracker::Initialize() {
  DCHECK_EQ(static_cast<TabStatsTracker*>(nullptr), g_instance);
  g_instance = new TabStatsTracker;
}

// static
TabStatsTracker* TabStatsTracker::GetInstance() {
  return g_instance;
}

TabStatsTracker::TabStatsTracker() : total_tabs_count_(0U), browser_count_(0U) {
  // Get the list of existing browsers/tabs. There shouldn't be any if this is
  // initialized at startup but this will ensure that the count stay accurate if
  // the initialization gets moved to after the creation of the first tab.
  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list) {
    ++browser_count_;
    browser->tab_strip_model()->AddObserver(this);
    total_tabs_count_ += browser->tab_strip_model()->count();
  }
  browser_list->AddObserver(this);
}

TabStatsTracker::~TabStatsTracker() {}

void TabStatsTracker::OnBrowserAdded(Browser* browser) {
  ++browser_count_;
  browser->tab_strip_model()->AddObserver(this);
}

void TabStatsTracker::OnBrowserRemoved(Browser* browser) {
  --browser_count_;
  browser->tab_strip_model()->RemoveObserver(this);
}

void TabStatsTracker::TabInsertedAt(TabStripModel* model,
                                    content::WebContents* web_contents,
                                    int index,
                                    bool foreground) {
  ++total_tabs_count_;
}

void TabStatsTracker::TabClosingAt(TabStripModel* model,
                                   content::WebContents* web_contents,
                                   int index) {
  --total_tabs_count_;
}

}  // namespace metrics
