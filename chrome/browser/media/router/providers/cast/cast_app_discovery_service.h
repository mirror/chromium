// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_CAST_APP_DISCOVERY_SERVICE_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_CAST_APP_DISCOVERY_SERVICE_H_

#include "base/containers/flat_map.h"

#include "chrome/browser/media/router/providers/cast/parsed_media_source.h"
#include "chrome/common/media_router/media_sink.h"
#include "chrome/common/media_router/media_source.h"

namespace media_router {

class CastMediaSinkService;
class MediaSinksObserver;

class CastAppDiscoveryService {
 public:
  CastAppDiscoveryService();
  ~CastAppDiscoveryService();

  void StartObservingMediaSinks(const MediaSource& source);
  void StopObservingMediaSinks(const MediaSource& source);

 private:
  // Supplies sinks to this class.
  // TODO: set to nonnull value
  CastMediaSinkService* const media_sink_service_ = nullptr;

  base::flat_map<MediaSource::Id, ParsedMediaSource> registered_sources_;
  base::flat_map<std::string, int> observer_count_by_app_id_;
  // XXX: MediaSink::Compare?
  // base::flat_map<std::string, base::flat_map<MediaSink::Id, MediaSink>>
  //    available_sinks_by_app_id_;

  DISALLOW_COPY_AND_ASSIGN(CastAppDiscoveryService);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_CAST_APP_DISCOVERY_SERVICE_H_
