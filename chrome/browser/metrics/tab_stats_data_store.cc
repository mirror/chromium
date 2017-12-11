// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/tab_stats_data_store.h"

#include <algorithm>
#include <utility>

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"

namespace metrics {

TabStatsDataStore::TabsStats::TabsStats()
    : total_tab_count(0U),
      total_tab_count_max(0U),
      max_tab_per_window(0U),
      window_count(0U),
      window_count_max(0U) {}

TabStatsDataStore::TabStatsDataStore(PrefService* pref_service)
    : pref_service_(pref_service) {
  DCHECK(pref_service != nullptr);
  tab_stats_.total_tab_count_max =
      pref_service->GetInteger(prefs::kTabStatsTotalTabCountMax);
  tab_stats_.max_tab_per_window =
      pref_service->GetInteger(prefs::kTabStatsMaxTabsPerWindow);
  tab_stats_.window_count_max =
      pref_service->GetInteger(prefs::kTabStatsWindowCountMax);
}

TabStatsDataStore::~TabStatsDataStore() {}

void TabStatsDataStore::OnWindowAdded() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  tab_stats_.window_count++;
  UpdateWindowCountMaxIfNeeded();
}

void TabStatsDataStore::OnWindowRemoved() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_GT(tab_stats_.window_count, 0U);
  tab_stats_.window_count--;
}

void TabStatsDataStore::OnTabAdded(content::WebContents* web_contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(web_contents);
  ++tab_stats_.total_tab_count;
  existing_tabs_.insert(web_contents);
  for (auto& interval_map : interval_maps_)
    AddTabToInterval(interval_map.second.get(), web_contents, false);
  UpdateTotalTabCountMaxIfNeeded();
}

void TabStatsDataStore::OnTabRemoved(content::WebContents* web_contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(web_contents);
  DCHECK_GT(tab_stats_.total_tab_count, 0U);
  --tab_stats_.total_tab_count;
  existing_tabs_.erase(web_contents);
  for (auto& interval_map : interval_maps_) {
    auto iter = interval_map.second->find(web_contents);
    DCHECK(iter != interval_map.second->end());
    iter->second.exists_after_interval = false;
  }
}

void TabStatsDataStore::OnTabReplaced(content::WebContents* old_contents,
                                      content::WebContents* new_contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(old_contents);
  DCHECK(new_contents);
  DCHECK(existing_tabs_.find(old_contents) != existing_tabs_.end());
  DCHECK_GT(tab_stats_.total_tab_count, 0U);
  existing_tabs_.erase(old_contents);
  existing_tabs_.insert(new_contents);
  for (auto& interval_map : interval_maps_) {
    auto iter = interval_map.second->find(old_contents);
    DCHECK(iter != interval_map.second->end());
    interval_map.second->insert(
        std::make_pair(new_contents, std::move(iter->second)));
    interval_map.second->erase(iter);
  }
}

void TabStatsDataStore::UpdateMaxTabsPerWindowIfNeeded(size_t value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (value <= tab_stats_.max_tab_per_window)
    return;
  tab_stats_.max_tab_per_window = value;
  pref_service_->SetInteger(prefs::kTabStatsMaxTabsPerWindow, value);
}

void TabStatsDataStore::ResetMaximumsToCurrentState() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Set the maximums to 0 and call the Update* functions to reset it to the
  // current value and update the pref registry.
  tab_stats_.max_tab_per_window = 0;
  tab_stats_.window_count_max = 0;
  tab_stats_.total_tab_count_max = 0;
  UpdateTotalTabCountMaxIfNeeded();
  UpdateWindowCountMaxIfNeeded();

  // Iterates over the list of browsers to find the one with the maximum number
  // of tabs opened.
  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list) {
    UpdateMaxTabsPerWindowIfNeeded(
        static_cast<size_t>(browser->tab_strip_model()->count()));
  }
}

void TabStatsDataStore::OnTabChangedEvent(content::WebContents* web_contents) {
  for (auto& interval_map : interval_maps_) {
    auto iter = interval_map.second->find(web_contents);
    DCHECK(iter != interval_map.second->end());
    iter->second.visible_or_audible_during_interval |=
        web_contents->WasRecentlyAudible();
  }
}

void TabStatsDataStore::OnTabInteraction(content::WebContents* web_contents) {
  // Mark the tab as interacted with in all the intervals.
  for (auto& interval_map : interval_maps_) {
    auto iter = interval_map.second->find(web_contents);
    DCHECK(iter != interval_map.second->end());
    iter->second.interacted_during_interval = true;
  }
}

void TabStatsDataStore::OnTabVisible(content::WebContents* web_contents) {
  // Mark the tab as visible in all the intervals.
  for (auto& interval_map : interval_maps_) {
    auto iter = interval_map.second->find(web_contents);
    DCHECK(iter != interval_map.second->end());
    iter->second.visible_or_audible_during_interval = true;
  }
}

void TabStatsDataStore::AddInterval(size_t interval_time_in_sec) {
  DCHECK(interval_maps_.find(interval_time_in_sec) == interval_maps_.end());
  // Creates the interval and initialize its data.
  std::unique_ptr<TabsStateDuringIntervalMap> interval_map =
      base::MakeUnique<TabsStateDuringIntervalMap>();
  ResetIntervalData(interval_map.get());
  interval_maps_[interval_time_in_sec] = std::move(interval_map);
}

TabStatsDataStore::TabsStateDuringIntervalMap*
TabStatsDataStore::GetIntervalMap(size_t interval_time_in_sec) {
  auto iter = interval_maps_.find(interval_time_in_sec);
  if (iter == interval_maps_.end())
    return nullptr;
  return iter->second.get();
}

void TabStatsDataStore::ResetIntervalData(
    TabsStateDuringIntervalMap* interval_map) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(interval_map != nullptr);
  interval_map->clear();
  for (content::WebContents* web_contents : existing_tabs_)
    AddTabToInterval(interval_map, web_contents, true);
}

void TabStatsDataStore::UpdateTotalTabCountMaxIfNeeded() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (tab_stats_.total_tab_count <= tab_stats_.total_tab_count_max)
    return;
  tab_stats_.total_tab_count_max = tab_stats_.total_tab_count;
  pref_service_->SetInteger(prefs::kTabStatsTotalTabCountMax,
                            tab_stats_.total_tab_count_max);
}

void TabStatsDataStore::UpdateWindowCountMaxIfNeeded() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (tab_stats_.window_count <= tab_stats_.window_count_max)
    return;
  tab_stats_.window_count_max = tab_stats_.window_count;
  pref_service_->SetInteger(prefs::kTabStatsWindowCountMax,
                            tab_stats_.window_count_max);
}

void TabStatsDataStore::AddTabToInterval(
    TabsStateDuringIntervalMap* interval_map,
    content::WebContents* web_contents,
    bool existed_before_interval) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(interval_map != nullptr);
  DCHECK(web_contents != nullptr);
  bool visible_or_audible =
      web_contents->IsVisible() || web_contents->IsCurrentlyAudible();
  (*interval_map)[web_contents] = {
      existed_before_interval,
      true,  // exists_after_interval
      visible_or_audible,
      false  // interacted_during_interval
  };
}

}  // namespace metrics
