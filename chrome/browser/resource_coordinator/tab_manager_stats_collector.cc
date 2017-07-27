// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_manager_stats_collector.h"

#include "base/metrics/histogram_macros.h"
#include "base/values.h"
#include "chrome/browser/resource_coordinator/tab_manager.h"
#include "chrome/browser/resource_coordinator/tab_manager_web_contents_data.h"
#include "content/public/browser/web_contents.h"

namespace resource_coordinator {

TabManagerStatsCollector::TabManagerStatsCollector(TabManager* tab_manager)
    : tab_manager_(tab_manager) {}

TabManagerStatsCollector::~TabManagerStatsCollector() = default;

void TabManagerStatsCollector::RecordSwitchToTab(
    content::WebContents* contents) const {
  if (tab_manager_->IsSessionRestoreLoadingTabs()) {
    auto* data = TabManager::WebContentsData::FromWebContents(contents);
    DCHECK(data);
    UMA_HISTOGRAM_ENUMERATION("TabManager.SessionRestore.SwitchToTab",
                              data->tab_loading_state(), TAB_LOADING_STATE_MAX);
  }
}

void TabManagerStatsCollector::RecordExpectedTaskQueueingDuration(
    content::WebContents* web_contents,
    std::unique_ptr<base::Value> queueing_time_millis) const {
  if (tab_manager_->IsSessionRestoreLoadingTabs() &&
      web_contents->IsVisible()) {
    UMA_HISTOGRAM_TIMES(
        "TabManager.Experimental.SessionRestore.ForegroundTab."
        "ExpectedTaskQueueingDuration",
        base::TimeDelta::FromMillisecondsD(queueing_time_millis->GetDouble()));
  }
}

}  // namespace resource_coordinator
