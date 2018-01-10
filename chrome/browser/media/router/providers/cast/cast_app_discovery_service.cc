// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/providers/cast/cast_app_discovery_service.h"

#include "chrome/browser/media/router/discovery/mdns/cast_media_sink_service.h"
#include "chrome/browser/media/router/media_sinks_observer.h"
#include "chrome/common/media_router/media_source_helper.h"

namespace media_router {

CastAppDiscoveryService::CastAppDiscoveryService() {}
CastAppDiscoveryService::~CastAppDiscoveryService() = default;

void CastAppDiscoveryService::StartObservingMediaSinks(
    const MediaSource& source) {
  const MediaSource::Id& source_id = source.id();
  if (base::ContainsKey(registered_sources_, source_id))
    return;

  std::unique_ptr<ParsedMediaSource> parsed = ParsedMediaSource::From(source);
  if (!parsed)
    return;

  // A broadcast request is not an actual sink query; it is used to send a
  // app precache message to receivers.
  if (parsed->broadcast_request) {
    // TODO(imcheng): Implement broadcast.
    return;
  }

  registered_sources_.emplace(source_id, *parsed);
  std::vector<std::string> new_app_ids;
  for (const auto& app_info : parsed->app_infos) {
    const auto& app_id = app_info.app_id;
    if (++observer_count_by_app_id_[app_id] == 1)
      new_app_ids.push_back(app_id);

    /*
        // We may have results from previous queries.
        may_have_cached_results =
            may_have_cached_results ||
            base::ContainsKey(available_sinks_by_app_id_, app_id);
            */
  }
  media_sink_service_->StartObservingMediaSinks(std::move(new_app_ids));
}

void CastAppDiscoveryService::StopObservingMediaSinks(
    const MediaSource& source) {
  const MediaSource::Id& source_id = source.id();
  auto it = registered_sources_.find(source_id);
  if (it == registered_sources_.end())
    return;

  std::vector<std::string> app_ids_to_remove;
  for (const auto& app_info : it->second.app_infos) {
    const std::string& app_id = app_info.app_id;
    auto count_it = observer_count_by_app_id_.find(app_id);
    DCHECK(count_it != observer_count_by_app_id_.end());
    if (--(count_it->second) == 0) {
      app_ids_to_remove.push_back(app_id);
      observer_count_by_app_id_.erase(count_it);
    }
  }

  if (!app_ids_to_remove.empty()) {
    media_sink_service_->StopObservingMediaSinks(std::move(app_ids_to_remove));
  }

  registered_sources_.erase(it);
}

}  // namespace media_router
