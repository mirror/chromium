// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/providers/cast/cast_media_route_provider.h"

#include "base/stl_util.h"
#include "chrome/common/media_router/providers/cast/cast_media_source.h"
#include "url/origin.h"

namespace media_router {

CastMediaRouteProvider::CastMediaRouteProvider(
    mojom::MediaRouteProviderRequest request,
    mojom::MediaRouterPtr media_router,
    CastAppDiscoveryService* app_discovery_service)
    : binding_(this, std::move(request)),
      media_router_(std::move(media_router)),
      app_discovery_service_(app_discovery_service) {
  DCHECK(app_discovery_service_);
}

CastMediaRouteProvider::~CastMediaRouteProvider() = default;

void CastMediaRouteProvider::CreateRoute(const std::string& media_source,
                                         const std::string& sink_id,
                                         const std::string& presentation_id,
                                         const url::Origin& origin,
                                         int32_t tab_id,
                                         base::TimeDelta timeout,
                                         bool incognito,
                                         CreateRouteCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(
      base::nullopt, std::string("Not implemented"),
      RouteRequestResult::ResultCode::NO_SUPPORTED_PROVIDER);
}

void CastMediaRouteProvider::JoinRoute(const std::string& media_source,
                                       const std::string& presentation_id,
                                       const url::Origin& origin,
                                       int32_t tab_id,
                                       base::TimeDelta timeout,
                                       bool incognito,
                                       JoinRouteCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(
      base::nullopt, std::string("Not implemented"),
      RouteRequestResult::ResultCode::NO_SUPPORTED_PROVIDER);
}

void CastMediaRouteProvider::ConnectRouteByRouteId(
    const std::string& media_source,
    const std::string& route_id,
    const std::string& presentation_id,
    const url::Origin& origin,
    int32_t tab_id,
    base::TimeDelta timeout,
    bool incognito,
    ConnectRouteByRouteIdCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(
      base::nullopt, std::string("Not implemented"),
      RouteRequestResult::ResultCode::NO_SUPPORTED_PROVIDER);
}

void CastMediaRouteProvider::TerminateRoute(const std::string& route_id,
                                            TerminateRouteCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(
      std::string("Not implemented"),
      RouteRequestResult::ResultCode::NO_SUPPORTED_PROVIDER);
}

void CastMediaRouteProvider::SendRouteMessage(
    const std::string& media_route_id,
    const std::string& message,
    SendRouteMessageCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(false);
}

void CastMediaRouteProvider::SendRouteBinaryMessage(
    const std::string& media_route_id,
    const std::vector<uint8_t>& data,
    SendRouteBinaryMessageCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(false);
}

void CastMediaRouteProvider::StartObservingMediaSinks(
    const std::string& media_source) {
  if (base::ContainsKey(sink_queries_, media_source))
    return;

  std::unique_ptr<CastMediaSource> cast_source =
      CastMediaSource::From(media_source);
  if (!cast_source)
    return;

  // A broadcast request is not an actual sink query; it is used to send a
  // app precache message to receivers.
  if (cast_source->broadcast_request()) {
    // TODO(imcheng): Implement broadcast, or figure out a re-design that does
    // not use sink queries.
    return;
  }

  sink_queries_[media_source] =
      app_discovery_service_->StartObservingMediaSinks(
          *cast_source,
          base::BindRepeating(&CastMediaRouteProvider::OnSinkQueryUpdated,
                              base::Unretained(this)));
}

void CastMediaRouteProvider::StopObservingMediaSinks(
    const std::string& media_source) {
  sink_queries_.erase(media_source);
}

void CastMediaRouteProvider::StartObservingMediaRoutes(
    const std::string& media_source) {
  NOTIMPLEMENTED();
}

void CastMediaRouteProvider::StopObservingMediaRoutes(
    const std::string& media_source) {
  NOTIMPLEMENTED();
}

void CastMediaRouteProvider::StartListeningForRouteMessages(
    const std::string& route_id) {
  NOTIMPLEMENTED();
}

void CastMediaRouteProvider::StopListeningForRouteMessages(
    const std::string& route_id) {
  NOTIMPLEMENTED();
}

void CastMediaRouteProvider::DetachRoute(const std::string& route_id) {
  NOTIMPLEMENTED();
}

void CastMediaRouteProvider::EnableMdnsDiscovery() {
  NOTIMPLEMENTED();
}

void CastMediaRouteProvider::UpdateMediaSinks(const std::string& media_source) {
  NOTIMPLEMENTED();
}

void CastMediaRouteProvider::SearchSinks(
    const std::string& sink_id,
    const std::string& media_source,
    mojom::SinkSearchCriteriaPtr search_criteria,
    SearchSinksCallback callback) {
  std::move(callback).Run(std::string());
}

void CastMediaRouteProvider::ProvideSinks(
    const std::string& provider_name,
    const std::vector<media_router::MediaSinkInternal>& sinks) {
  NOTIMPLEMENTED();
}

void CastMediaRouteProvider::CreateMediaRouteController(
    const std::string& route_id,
    mojom::MediaControllerRequest media_controller,
    mojom::MediaStatusObserverPtr observer,
    CreateMediaRouteControllerCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(false);
}

void CastMediaRouteProvider::OnSinkQueryUpdated(
    const MediaSource::Id& source_id,
    const std::vector<MediaSinkInternal>& sinks,
    const std::vector<url::Origin>& origins) {
  // Use EXTENSION as provider ID for now since CreateRoute is still handled by
  // the extension.
  media_router_->OnSinksReceived(MediaRouteProviderId::EXTENSION, source_id,
                                 sinks, origins);
}

}  // namespace media_router
