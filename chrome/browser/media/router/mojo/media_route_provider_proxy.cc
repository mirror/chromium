// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/mojo/media_route_provider_proxy.h"

#include <string>
#include <utility>
#include <vector>

#include "chrome/browser/media/router/event_page_request_manager.h"
#include "chrome/browser/media/router/event_page_request_manager_factory.h"
#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/browser/media/router/mojo/media_router_mojo_impl.h"

namespace media_router {

MediaRouteProviderProxy::MediaRouteProviderProxy(
    content::BrowserContext* context,
    MediaRouterMojoImpl* media_router)
    : request_manager_(
          EventPageRequestManagerFactory::GetApiForBrowserContext(context)),
      media_router_(media_router),
      weak_factory_(this) {}

MediaRouteProviderProxy::~MediaRouteProviderProxy() = default;

void MediaRouteProviderProxy::RegisterMediaRouteProvider(
    mojom::MediaRouteProviderPtr media_route_provider_ptr,
    mojom::MediaRouter::RegisterMediaRouteProviderCallback callback) {
  media_route_provider_ = std::move(media_route_provider_ptr);
  media_route_provider_.set_connection_error_handler(base::Bind(
      &MediaRouteProviderProxy::OnConnectionError, base::Unretained(this)));

  auto config = mojom::MediaRouteProviderConfig::New();
  // Enabling browser side discovery means disabling extension side discovery.
  // We are migrating discovery from the external Media Route Provider to the
  // Media Router (crbug.com/687383), so we need to disable it in the provider.
  config->enable_dial_discovery = !media_router::DialLocalDiscoveryEnabled();
  config->enable_cast_discovery = !media_router::CastDiscoveryEnabled();
  std::move(callback).Run(media_router_->instance_id(), std::move(config));

  request_manager_->OnMojoConnectionsReady();
  media_router_->SyncStateToMediaRouteProvider();
}

void MediaRouteProviderProxy::CreateRoute(
    const std::string& media_source,
    const std::string& sink_id,
    const std::string& original_presentation_id,
    const url::Origin& origin,
    int32_t tab_id,
    base::TimeDelta timeout,
    bool incognito,
    CreateRouteCallback callback) {
  request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouteProviderProxy::DoCreateRoute,
                     weak_factory_.GetWeakPtr(), media_source, sink_id,
                     original_presentation_id, origin, tab_id, timeout,
                     incognito, std::move(callback)),
      MediaRouteProviderWakeReason::CREATE_ROUTE);
}

void MediaRouteProviderProxy::JoinRoute(const std::string& media_source,
                                        const std::string& presentation_id,
                                        const url::Origin& origin,
                                        int32_t tab_id,
                                        base::TimeDelta timeout,
                                        bool incognito,
                                        JoinRouteCallback callback) {
  request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouteProviderProxy::DoJoinRoute,
                     weak_factory_.GetWeakPtr(), media_source, presentation_id,
                     origin, tab_id, timeout, incognito, std::move(callback)),
      MediaRouteProviderWakeReason::JOIN_ROUTE);
}

void MediaRouteProviderProxy::ConnectRouteByRouteId(
    const std::string& media_source,
    const std::string& route_id,
    const std::string& presentation_id,
    const url::Origin& origin,
    int32_t tab_id,
    base::TimeDelta timeout,
    bool incognito,
    ConnectRouteByRouteIdCallback callback) {
  request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouteProviderProxy::DoConnectRouteByRouteId,
                     weak_factory_.GetWeakPtr(), media_source, route_id,
                     presentation_id, origin, tab_id, timeout, incognito,
                     std::move(callback)),
      MediaRouteProviderWakeReason::CONNECT_ROUTE_BY_ROUTE_ID);
}

void MediaRouteProviderProxy::TerminateRoute(const std::string& route_id,
                                             TerminateRouteCallback callback) {
  DVLOG(2) << "TerminateRoute " << route_id;
  request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouteProviderProxy::DoTerminateRoute,
                     weak_factory_.GetWeakPtr(), route_id, std::move(callback)),
      MediaRouteProviderWakeReason::TERMINATE_ROUTE);
}

void MediaRouteProviderProxy::SendRouteMessage(
    const std::string& media_route_id,
    const std::string& message,
    SendRouteMessageCallback callback) {
  request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouteProviderProxy::DoSendRouteMessage,
                     weak_factory_.GetWeakPtr(), media_route_id, message,
                     std::move(callback)),
      MediaRouteProviderWakeReason::SEND_SESSION_MESSAGE);
}

void MediaRouteProviderProxy::SendRouteBinaryMessage(
    const std::string& media_route_id,
    const std::vector<uint8_t>& data,
    SendRouteBinaryMessageCallback callback) {
  request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouteProviderProxy::DoSendRouteBinaryMessage,
                     weak_factory_.GetWeakPtr(), media_route_id, data,
                     std::move(callback)),
      MediaRouteProviderWakeReason::SEND_SESSION_BINARY_MESSAGE);
}

void MediaRouteProviderProxy::StartObservingMediaSinks(
    const std::string& media_source) {
  request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouteProviderProxy::DoStartObservingMediaSinks,
                     weak_factory_.GetWeakPtr(), media_source),
      MediaRouteProviderWakeReason::START_OBSERVING_MEDIA_SINKS);
}

void MediaRouteProviderProxy::StopObservingMediaSinks(
    const std::string& media_source) {
  request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouteProviderProxy::DoStopObservingMediaSinks,
                     weak_factory_.GetWeakPtr(), media_source),
      MediaRouteProviderWakeReason::STOP_OBSERVING_MEDIA_ROUTES);
}

void MediaRouteProviderProxy::StartObservingMediaRoutes(
    const std::string& media_source) {
  request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouteProviderProxy::DoStartObservingMediaRoutes,
                     weak_factory_.GetWeakPtr(), media_source),
      MediaRouteProviderWakeReason::START_OBSERVING_MEDIA_ROUTES);
}

void MediaRouteProviderProxy::StopObservingMediaRoutes(
    const std::string& media_source) {
  request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouteProviderProxy::DoStopObservingMediaRoutes,
                     weak_factory_.GetWeakPtr(), media_source),
      MediaRouteProviderWakeReason::STOP_OBSERVING_MEDIA_ROUTES);
}

void MediaRouteProviderProxy::StartListeningForRouteMessages(
    const std::string& route_id) {
  request_manager_->RunOrDefer(
      base::Bind(&MediaRouteProviderProxy::DoStartListeningForRouteMessages,
                 weak_factory_.GetWeakPtr(), route_id),
      MediaRouteProviderWakeReason::START_LISTENING_FOR_ROUTE_MESSAGES);
}

void MediaRouteProviderProxy::StopListeningForRouteMessages(
    const std::string& route_id) {
  request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouteProviderProxy::DoStopListeningForRouteMessages,
                     weak_factory_.GetWeakPtr(), route_id),
      MediaRouteProviderWakeReason::STOP_LISTENING_FOR_ROUTE_MESSAGES);
}

void MediaRouteProviderProxy::DetachRoute(const std::string& route_id) {
  request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouteProviderProxy::DoDetachRoute,
                     weak_factory_.GetWeakPtr(), route_id),
      MediaRouteProviderWakeReason::DETACH_ROUTE);
}

void MediaRouteProviderProxy::EnableMdnsDiscovery() {
  request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouteProviderProxy::DoEnableMdnsDiscovery,
                     weak_factory_.GetWeakPtr()),
      MediaRouteProviderWakeReason::ENABLE_MDNS_DISCOVERY);
}

void MediaRouteProviderProxy::UpdateMediaSinks(
    const std::string& media_source) {
  request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouteProviderProxy::DoUpdateMediaSinks,
                     weak_factory_.GetWeakPtr(), media_source),
      MediaRouteProviderWakeReason::UPDATE_MEDIA_SINKS);
}

void MediaRouteProviderProxy::SearchSinks(
    const std::string& sink_id,
    const std::string& media_source,
    mojom::SinkSearchCriteriaPtr search_criteria,
    SearchSinksCallback callback) {
  request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouteProviderProxy::DoSearchSinks,
                     weak_factory_.GetWeakPtr(), sink_id, media_source,
                     std::move(search_criteria), std::move(callback)),
      MediaRouteProviderWakeReason::SEARCH_SINKS);
}

void MediaRouteProviderProxy::ProvideSinks(
    const std::string& provider_name,
    const std::vector<media_router::MediaSinkInternal>& sinks) {
  DVLOG(1) << "OnDialMediaSinkDiscovered found " << sinks.size()
           << " devices...";
  request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouteProviderProxy::DoProvideSinks,
                     weak_factory_.GetWeakPtr(), provider_name, sinks),
      MediaRouteProviderWakeReason::PROVIDE_SINKS);
}

void MediaRouteProviderProxy::CreateMediaRouteController(
    const std::string& route_id,
    mojom::MediaControllerRequest media_controller,
    mojom::MediaStatusObserverPtr observer,
    CreateMediaRouteControllerCallback callback) {
  request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouteProviderProxy::DoCreateMediaRouteController,
                     weak_factory_.GetWeakPtr(), route_id,
                     std::move(media_controller), std::move(observer),
                     std::move(callback)),
      MediaRouteProviderWakeReason::CREATE_MEDIA_ROUTE_CONTROLLER);
}

void MediaRouteProviderProxy::OnConnectionError() {
  request_manager_->OnMojoConnectionError();
  media_route_provider_.reset();
}

void MediaRouteProviderProxy::DoCreateRoute(
    const std::string& media_source,
    const std::string& sink_id,
    const std::string& original_presentation_id,
    const url::Origin& origin,
    int32_t tab_id,
    base::TimeDelta timeout,
    bool incognito,
    CreateRouteCallback callback) {
  DVLOG(1) << "DoCreateRoute " << media_source << "=>" << sink_id
           << ", presentation ID: " << original_presentation_id;
  media_route_provider_->CreateRoute(media_source, sink_id,
                                     original_presentation_id, origin, tab_id,
                                     timeout, incognito, std::move(callback));
}

void MediaRouteProviderProxy::DoJoinRoute(const std::string& media_source,
                                          const std::string& presentation_id,
                                          const url::Origin& origin,
                                          int32_t tab_id,
                                          base::TimeDelta timeout,
                                          bool incognito,
                                          JoinRouteCallback callback) {
  DVLOG(1) << "DoJoinRoute " << media_source
           << ", presentation ID: " << presentation_id;
  media_route_provider_->JoinRoute(media_source, presentation_id, origin,
                                   tab_id, timeout, incognito,
                                   std::move(callback));
}

void MediaRouteProviderProxy::DoConnectRouteByRouteId(
    const std::string& media_source,
    const std::string& route_id,
    const std::string& presentation_id,
    const url::Origin& origin,
    int32_t tab_id,
    base::TimeDelta timeout,
    bool incognito,
    ConnectRouteByRouteIdCallback callback) {
  DVLOG(1) << "DoConnectRouteByRouteId " << media_source
           << ", route ID: " << route_id
           << ", presentation ID: " << presentation_id;

  media_route_provider_->ConnectRouteByRouteId(
      media_source, route_id, presentation_id, origin, tab_id, timeout,
      incognito, std::move(callback));
}

void MediaRouteProviderProxy::DoTerminateRoute(
    const std::string& route_id,
    TerminateRouteCallback callback) {
  DVLOG(1) << "DoTerminateRoute " << route_id;
  media_route_provider_->TerminateRoute(route_id, std::move(callback));
}

void MediaRouteProviderProxy::DoSendRouteMessage(
    const std::string& media_route_id,
    const std::string& message,
    SendRouteMessageCallback callback) {
  DVLOG(1) << "DoSendRouteMessage " << media_route_id;
  media_route_provider_->SendRouteMessage(media_route_id, message,
                                          std::move(callback));
}

void MediaRouteProviderProxy::DoSendRouteBinaryMessage(
    const std::string& media_route_id,
    const std::vector<uint8_t>& data,
    SendRouteBinaryMessageCallback callback) {
  DVLOG(1) << "DoSendRouteBinaryMessage " << media_route_id;
  media_route_provider_->SendRouteBinaryMessage(media_route_id, data,
                                                std::move(callback));
}

void MediaRouteProviderProxy::DoStartObservingMediaSinks(
    const std::string& media_source) {
  DVLOG(1) << "DoStartObservingMediaSinks: " << media_source;
  if (media_router_->ShouldStartObservingSinks(media_source))
    media_route_provider_->StartObservingMediaSinks(media_source);
}

void MediaRouteProviderProxy::DoStopObservingMediaSinks(
    const std::string& media_source) {
  DVLOG(1) << "DoStopObservingMediaSinks: " << media_source;
  if (media_router_->ShouldStopObservingMediaSinks(media_source))
    media_route_provider_->StopObservingMediaSinks(media_source);
}

void MediaRouteProviderProxy::DoStartObservingMediaRoutes(
    const std::string& media_source) {
  DVLOG(1) << "DoStartObservingMediaRoutes: " << media_source;
  if (media_router_->ShouldStartObservingMediaRoutes(media_source))
    media_route_provider_->StartObservingMediaRoutes(media_source);
}

void MediaRouteProviderProxy::DoStopObservingMediaRoutes(
    const std::string& media_source) {
  DVLOG(1) << "DoStopObservingMediaRoutes: " << media_source;
  if (media_router_->ShouldStopObservingMediaRoutes(media_source))
    media_route_provider_->StopObservingMediaRoutes(media_source);
}

void MediaRouteProviderProxy::DoStartListeningForRouteMessages(
    const std::string& route_id) {
  DVLOG(1) << "DoStartListeningForRouteMessages";
  media_route_provider_->StartListeningForRouteMessages(route_id);
}

void MediaRouteProviderProxy::DoStopListeningForRouteMessages(
    const std::string& route_id) {
  DVLOG(1) << "StopListeningForRouteMessages";
  media_route_provider_->StopListeningForRouteMessages(route_id);
}

void MediaRouteProviderProxy::DoDetachRoute(const std::string& route_id) {
  DVLOG(1) << "DoDetachRoute " << route_id;
  media_route_provider_->DetachRoute(route_id);
}

void MediaRouteProviderProxy::DoEnableMdnsDiscovery() {
  DVLOG(1) << "DoEnsureMdnsDiscoveryEnabled";
  media_route_provider_->EnableMdnsDiscovery();
}

void MediaRouteProviderProxy::DoUpdateMediaSinks(
    const std::string& media_source) {
  DVLOG(1) << "DoUpdateMediaSinks: " << media_source;
  media_route_provider_->UpdateMediaSinks(media_source);
}

void MediaRouteProviderProxy::DoSearchSinks(
    const std::string& sink_id,
    const std::string& media_source,
    mojom::SinkSearchCriteriaPtr search_criteria,
    SearchSinksCallback callback) {
  DVLOG(1) << "SearchSinks";
  media_route_provider_->SearchSinks(
      sink_id, media_source, std::move(search_criteria), std::move(callback));
}

void MediaRouteProviderProxy::DoProvideSinks(
    const std::string& provider_name,
    const std::vector<media_router::MediaSinkInternal>& sinks) {
  DVLOG(1) << "DoProvideSinks";
  media_route_provider_->ProvideSinks(provider_name, sinks);
}

void MediaRouteProviderProxy::DoCreateMediaRouteController(
    const std::string& route_id,
    mojom::MediaControllerRequest media_controller,
    mojom::MediaStatusObserverPtr observer,
    CreateMediaRouteControllerCallback callback) {
  DVLOG(1) << "DoCreateMediaRouteController";
  if (!media_controller.is_pending() || !observer.is_bound())
    return;

  media_route_provider_->CreateMediaRouteController(
      route_id, std::move(media_controller), std::move(observer),
      std::move(callback));
}

}  // namespace media_router
