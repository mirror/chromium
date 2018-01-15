// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/mdns/app_availability_tracker.h"

namespace media_router {

AppAvailabilityTracker::AppAvailabilityTracker() {}
AppAvailabilityTracker::~AppAvailabilityTracker() = default;

bool AppAvailabilityTracker::HasSource(const ParsedMediaSource& source) const {
  return base::ContainsKey(registered_sources_, source.source_id);
}

base::flat_set<std::string> AppAvailabilityTracker::RegisterSource(
    const ParsedMediaSource& source) {
  if (HasSource(source))
    return base::flat_set<std::string>();

  registered_sources_.emplace(source.source_id, source);

  base::flat_set<std::string> new_app_ids;
  for (const auto& app_info : source.app_infos) {
    const auto& app_id = app_info.app_id;
    if (++observer_count_by_app_id_[app_id] == 1)
      new_app_ids.insert(app_id);
  }
  return new_app_ids;
}

void AppAvailabilityTracker::UnregisterSource(
    const MediaSource::Id& source_id) {
  auto it = registered_sources_.find(source_id);
  if (it == registered_sources_.end())
    return;

  for (const auto& app_info : it->second.app_infos) {
    const std::string& app_id = app_info.app_id;
    auto count_it = observer_count_by_app_id_.find(app_id);
    DCHECK(count_it != observer_count_by_app_id_.end());
    if (count_it->second == 1)
      observer_count_by_app_id_.erase(count_it);
  }

  registered_sources_.erase(it);
}

std::vector<ParsedMediaSource> AppAvailabilityTracker::UpdateAppAvailability(
    const MediaSink::Id& sink_id,
    const std::string& app_id,
    AppAvailability availability) {
  app_availabilities_[sink_id][app_id] = availability;
  bool updated = false;
  if (availability == AppAvailability::kAvailable)
    updated = available_sinks_by_app_id_[app_id].insert(sink_id).second;
  else
    updated = available_sinks_by_app_id_[app_id].erase(sink_id) != 0u;

  if (!updated)
    return std::vector<ParsedMediaSource>();

  std::vector<ParsedMediaSource> affected_sources;
  for (const auto& source : registered_sources_) {
    if (source.second.ContainsApp(app_id))
      affected_sources.push_back(source.second);
  }
  return affected_sources;
}

std::vector<ParsedMediaSource> AppAvailabilityTracker::RemoveResultsForSink(
    const MediaSink::Id& sink_id) {
  app_availabilities_.erase(sink_id);
  std::vector<std::string> affected_app_ids;
  for (auto& available_sinks : available_sinks_by_app_id_) {
    if (available_sinks.second.erase(sink_id) != 0u)
      affected_app_ids.push_back(available_sinks.first);
  }
  std::vector<ParsedMediaSource> affected_sources;
  for (const auto& source : registered_sources_) {
    for (const auto& app_id : affected_app_ids) {
      if (source.second.ContainsApp(app_id)) {
        affected_sources.push_back(source.second);
        break;
      }
    }
  }
  return affected_sources;
}

bool AppAvailabilityTracker::IsAvailabilityKnown(
    const MediaSink::Id& sink_id,
    const std::string& app_id) const {
  auto availabilities_it = app_availabilities_.find(sink_id);
  return availabilities_it == app_availabilities_.end() ||
         !base::ContainsKey(availabilities_it->second, app_id);
}

base::flat_set<std::string> AppAvailabilityTracker::GetRegisteredApps() const {
  base::flat_set<std::string> registered_apps;
  for (const auto& app_ids_and_count : observer_count_by_app_id_)
    registered_apps.insert(app_ids_and_count.first);

  return registered_apps;
}

base::flat_set<MediaSink::Id> AppAvailabilityTracker::GetAvailableSinks(
    const ParsedMediaSource& source) const {
  base::flat_set<MediaSink::Id> sink_ids;
  for (const auto& app_info : source.app_infos) {
    const std::string& app_id = app_info.app_id;
    auto it = available_sinks_by_app_id_.find(app_id);
    if (it != available_sinks_by_app_id_.end())
      sink_ids.insert(it->second.begin(), it->second.end());
  }
  return sink_ids;
}

}  // namespace media_router
