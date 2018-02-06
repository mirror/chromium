// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/providers/dial/dial_media_route_provider.h"

#include "base/stl_util.h"
#include "chrome/common/media_router/media_source_helper.h"
#include "url/origin.h"

namespace media_router {

DialMediaRouteProvider::DialMediaRouteProvider(
    mojom::MediaRouteProviderRequest request,
    mojom::MediaRouterPtr media_router,
    DialMediaSinkService* dial_media_sink_service)
    : binding_(this, std::move(request)),
      media_router_(std::move(media_router)),
      dial_media_sink_service_(dial_media_sink_service) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(dial_media_sink_service_);
  dial_media_sink_service_->RegisterOnSinkQueryUpdatedCallback(
      base::BindRepeating(&DialMediaRouteProvider::OnSinkQueryUpdated,
                          base::Unretained(this)));
}

DialMediaRouteProvider::~DialMediaRouteProvider() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  dial_media_sink_service_->UnregisterOnSinkQueryUpdatedCallback();
}

void DialMediaRouteProvider::CreateRoute(const std::string& media_source,
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

void DialMediaRouteProvider::JoinRoute(const std::string& media_source,
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

void DialMediaRouteProvider::ConnectRouteByRouteId(
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

void DialMediaRouteProvider::TerminateRoute(const std::string& route_id,
                                            TerminateRouteCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(
      std::string("Not implemented"),
      RouteRequestResult::ResultCode::NO_SUPPORTED_PROVIDER);
}

void DialMediaRouteProvider::SendRouteMessage(
    const std::string& media_route_id,
    const std::string& message,
    SendRouteMessageCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(false);
}

void DialMediaRouteProvider::SendRouteBinaryMessage(
    const std::string& media_route_id,
    const std::vector<uint8_t>& data,
    SendRouteBinaryMessageCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(false);
}

void DialMediaRouteProvider::StartObservingMediaSinks(
    const std::string& media_source) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(dial_media_sink_service_);
  if (!IsDialMediaSource(MediaSource(media_source)))
    return;

  dial_media_sink_service_->StartObservingMediaSinks(media_source);
}

void DialMediaRouteProvider::StopObservingMediaSinks(
    const std::string& media_source) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(dial_media_sink_service_);
  dial_media_sink_service_->StopObservingMediaSinks(media_source);
}

void DialMediaRouteProvider::StartObservingMediaRoutes(
    const std::string& media_source) {
  NOTIMPLEMENTED();
}

void DialMediaRouteProvider::StopObservingMediaRoutes(
    const std::string& media_source) {
  NOTIMPLEMENTED();
}

void DialMediaRouteProvider::StartListeningForRouteMessages(
    const std::string& route_id) {
  NOTIMPLEMENTED();
}

void DialMediaRouteProvider::StopListeningForRouteMessages(
    const std::string& route_id) {
  NOTIMPLEMENTED();
}

void DialMediaRouteProvider::DetachRoute(const std::string& route_id) {
  NOTIMPLEMENTED();
}

void DialMediaRouteProvider::EnableMdnsDiscovery() {
  NOTIMPLEMENTED();
}

void DialMediaRouteProvider::UpdateMediaSinks(const std::string& media_source) {
  NOTIMPLEMENTED();
}

void DialMediaRouteProvider::SearchSinks(
    const std::string& sink_id,
    const std::string& media_source,
    mojom::SinkSearchCriteriaPtr search_criteria,
    SearchSinksCallback callback) {
  std::move(callback).Run(std::string());
}

void DialMediaRouteProvider::ProvideSinks(
    const std::string& provider_name,
    const std::vector<media_router::MediaSinkInternal>& sinks) {
  NOTIMPLEMENTED();
}

void DialMediaRouteProvider::CreateMediaRouteController(
    const std::string& route_id,
    mojom::MediaControllerRequest media_controller,
    mojom::MediaStatusObserverPtr observer,
    CreateMediaRouteControllerCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(false);
}

void DialMediaRouteProvider::OnSinkQueryUpdated(
    const MediaSource::Id& source_id,
    const std::vector<MediaSinkInternal>& sinks,
    const std::vector<url::Origin>& origins) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  media_router_->OnSinksReceived(MediaRouteProviderId::DIAL, source_id, sinks,
                                 origins);
}

}  // namespace media_router
