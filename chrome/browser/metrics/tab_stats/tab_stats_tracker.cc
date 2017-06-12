// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/tab_stats/tab_stats_tracker.h"

#include "base/power_monitor/power_monitor.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

namespace metrics {

namespace {

TabStatsTracker* g_instance = nullptr;

}  // namespace

// static
void TabStatsTracker::Initialize() {
  g_instance = new TabStatsTracker;
  VLOG(1) << "Initializing the tab stats tracker.";

}

// static
bool TabStatsTracker::IsInitialized() {
  return g_instance != nullptr;
}

// static
TabStatsTracker* TabStatsTracker::Get() {
  DCHECK(g_instance);
  return g_instance;
}

void TabStatsTracker::OnBrowserAdded(Browser* browser) {
  browser->tab_strip_model()->AddObserver(this);
  VLOG(1) << "OnBrowserAdded.";
}

void TabStatsTracker::OnBrowserRemoved(Browser* browser) {
  browser->tab_strip_model()->RemoveObserver(this);
  VLOG(1) << "OnBrowserRemoved.";
}

void TabStatsTracker::TabInsertedAt(TabStripModel* model,
                                    content::WebContents* web_contents,
                                    int index,
                                    bool foreground) {
  VLOG(1) << "TabInsertedAt: " << index;
  total_tabs_count_++;
}

void TabStatsTracker::TabClosingAt(TabStripModel* model,
                                   content::WebContents* web_contents,
                                   int index) {
  VLOG(1) << "TabClosingAt: " << index;
  total_tabs_count_--;
}

void TabStatsTracker::OnSessionRestoreDone(int num_tabs) {
 VLOG(1) << "TabStatsTracker::OnSessionRestoreDone: " << num_tabs;
}

void TabStatsTracker::OnResume() {
 VLOG(1) << "TabStatsTracker::OnResume: " << total_tabs_count_;
}

TabStatsTracker::TabStatsTracker() : total_tabs_count_(0U) {
  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list)
    browser->tab_strip_model()->AddObserver(this);
  browser_list->AddObserver(this);
  on_session_restored_callback_subscription_ =
      SessionRestore::RegisterOnSessionRestoredCallback(
          base::Bind(&TabStatsTracker::OnSessionRestoreDone,
                     base::Unretained(this)));
  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  power_monitor->AddObserver(this);
}

TabStatsTracker::~TabStatsTracker() {}

}  // namespace metrics
