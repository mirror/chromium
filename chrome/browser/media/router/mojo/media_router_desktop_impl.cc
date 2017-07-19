// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/mojo/media_router_desktop_impl.h"

#include "chrome/browser/media/router/event_page_request_manager.h"
#include "chrome/browser/media/router/event_page_request_manager_factory.h"
#include "chrome/browser/media/router/media_router_factory.h"
#include "chrome/common/media_router/media_source_helper.h"
#include "extensions/common/extension.h"

namespace media_router {

MediaRouterDesktopImpl::~MediaRouterDesktopImpl() = default;

// static
void MediaRouterDesktopImpl::BindToRequest(
    const extensions::Extension* extension,
    content::BrowserContext* context,
    const service_manager::BindSourceInfo& source_info,
    mojom::MediaRouterRequest request) {
  MediaRouterDesktopImpl* impl = static_cast<MediaRouterDesktopImpl*>(
      MediaRouterFactory::GetApiForBrowserContext(context));
  DCHECK(impl);

  impl->BindToMojoRequest(std::move(request), *extension);
}

void MediaRouterDesktopImpl::OnUserGesture() {
  MediaRouterMojoImpl::OnUserGesture();
  // Allow MRPM to intelligently update sinks and observers by passing in a
  // media source.
  UpdateMediaSinks(MediaSourceForDesktop().id());

#if defined(OS_WIN)
  EnsureMdnsDiscoveryEnabled();
#endif
}

void MediaRouterDesktopImpl::DoCreateRoute(
    const MediaSource::Id& source_id,
    const MediaSink::Id& sink_id,
    const url::Origin& origin,
    int tab_id,
    std::vector<MediaRouteResponseCallback> callbacks,
    base::TimeDelta timeout,
    bool incognito) {
  if (event_page_request_manager_->mojo_connections_ready()) {
    MediaRouterMojoImpl::DoCreateRoute(source_id, sink_id, origin, tab_id,
                                       std::move(callbacks), timeout,
                                       incognito);
    return;
  }
  event_page_request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouterDesktopImpl::DoCreateRoute,
                     weak_factory_.GetWeakPtr(), source_id, sink_id, origin,
                     tab_id, std::move(callbacks), timeout, incognito),
      MediaRouteProviderWakeReason::CREATE_ROUTE);
}

void MediaRouterDesktopImpl::DoJoinRoute(
    const MediaSource::Id& source_id,
    const std::string& presentation_id,
    const url::Origin& origin,
    int tab_id,
    std::vector<MediaRouteResponseCallback> callbacks,
    base::TimeDelta timeout,
    bool incognito) {
  if (event_page_request_manager_->mojo_connections_ready()) {
    MediaRouterMojoImpl::DoJoinRoute(source_id, presentation_id, origin, tab_id,
                                     std::move(callbacks), timeout, incognito);
    return;
  }
  event_page_request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouterDesktopImpl::DoJoinRoute,
                     weak_factory_.GetWeakPtr(), source_id, presentation_id,
                     origin, tab_id, std::move(callbacks), timeout, incognito),
      MediaRouteProviderWakeReason::JOIN_ROUTE);
}

void MediaRouterDesktopImpl::DoConnectRouteByRouteId(
    const MediaSource::Id& source_id,
    const MediaRoute::Id& route_id,
    const url::Origin& origin,
    int tab_id,
    std::vector<MediaRouteResponseCallback> callbacks,
    base::TimeDelta timeout,
    bool incognito) {
  if (event_page_request_manager_->mojo_connections_ready()) {
    MediaRouterMojoImpl::DoConnectRouteByRouteId(source_id, route_id, origin,
                                                 tab_id, std::move(callbacks),
                                                 timeout, incognito);
    return;
  }
  event_page_request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouterDesktopImpl::DoConnectRouteByRouteId,
                     weak_factory_.GetWeakPtr(), source_id, route_id, origin,
                     tab_id, std::move(callbacks), timeout, incognito),
      MediaRouteProviderWakeReason::CONNECT_ROUTE_BY_ROUTE_ID);
}

void MediaRouterDesktopImpl::DoTerminateRoute(const MediaRoute::Id& route_id) {
  if (event_page_request_manager_->mojo_connections_ready()) {
    MediaRouterMojoImpl::DoTerminateRoute(route_id);
    return;
  }
  event_page_request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouterDesktopImpl::DoTerminateRoute,
                     weak_factory_.GetWeakPtr(), route_id),
      MediaRouteProviderWakeReason::TERMINATE_ROUTE);
}

void MediaRouterDesktopImpl::DoDetachRoute(const MediaRoute::Id& route_id) {
  if (event_page_request_manager_->mojo_connections_ready()) {
    MediaRouterMojoImpl::DoDetachRoute(route_id);
    return;
  }
  event_page_request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouterDesktopImpl::DoDetachRoute,
                     weak_factory_.GetWeakPtr(), route_id),
      MediaRouteProviderWakeReason::DETACH_ROUTE);
}

void MediaRouterDesktopImpl::DoSendSessionMessage(
    const MediaRoute::Id& route_id,
    const std::string& message,
    SendRouteMessageCallback callback) {
  if (event_page_request_manager_->mojo_connections_ready()) {
    MediaRouterMojoImpl::DoSendSessionMessage(route_id, message,
                                              std::move(callback));
    return;
  }
  event_page_request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouterDesktopImpl::DoSendSessionMessage,
                     weak_factory_.GetWeakPtr(), route_id, message,
                     std::move(callback)),
      MediaRouteProviderWakeReason::SEND_SESSION_MESSAGE);
}

void MediaRouterDesktopImpl::DoSendSessionBinaryMessage(
    const MediaRoute::Id& route_id,
    std::unique_ptr<std::vector<uint8_t>> data,
    SendRouteMessageCallback callback) {
  if (event_page_request_manager_->mojo_connections_ready()) {
    MediaRouterMojoImpl::DoSendSessionBinaryMessage(route_id, std::move(data),
                                                    std::move(callback));
    return;
  }
  event_page_request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouterDesktopImpl::DoSendSessionBinaryMessage,
                     weak_factory_.GetWeakPtr(), route_id,
                     base::Passed(std::move(data)), std::move(callback)),
      MediaRouteProviderWakeReason::SEND_SESSION_BINARY_MESSAGE);
}

void MediaRouterDesktopImpl::DoStartListeningForRouteMessages(
    const MediaRoute::Id& route_id) {
  if (event_page_request_manager_->mojo_connections_ready()) {
    MediaRouterMojoImpl::DoStartListeningForRouteMessages(route_id);
    return;
  }
  event_page_request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouterDesktopImpl::DoStartListeningForRouteMessages,
                     weak_factory_.GetWeakPtr(), route_id),
      MediaRouteProviderWakeReason::START_LISTENING_FOR_ROUTE_MESSAGES);
}

void MediaRouterDesktopImpl::DoStopListeningForRouteMessages(
    const MediaRoute::Id& route_id) {
  if (event_page_request_manager_->mojo_connections_ready()) {
    MediaRouterMojoImpl::DoStopListeningForRouteMessages(route_id);
    return;
  }
  event_page_request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouterDesktopImpl::DoStopListeningForRouteMessages,
                     weak_factory_.GetWeakPtr(), route_id),
      MediaRouteProviderWakeReason::STOP_LISTENING_FOR_ROUTE_MESSAGES);
}

void MediaRouterDesktopImpl::DoStartObservingMediaSinks(
    const MediaSource::Id& source_id) {
  if (event_page_request_manager_->mojo_connections_ready()) {
    MediaRouterMojoImpl::DoStartObservingMediaSinks(source_id);
    return;
  }
  event_page_request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouterDesktopImpl::DoStartObservingMediaSinks,
                     weak_factory_.GetWeakPtr(), source_id),
      MediaRouteProviderWakeReason::START_OBSERVING_MEDIA_SINKS);
}

void MediaRouterDesktopImpl::DoStopObservingMediaSinks(
    const MediaSource::Id& source_id) {
  if (event_page_request_manager_->mojo_connections_ready()) {
    MediaRouterMojoImpl::DoStopObservingMediaSinks(source_id);
    return;
  }
  event_page_request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouterDesktopImpl::DoStopObservingMediaSinks,
                     weak_factory_.GetWeakPtr(), source_id),
      MediaRouteProviderWakeReason::STOP_OBSERVING_MEDIA_SINKS);
}

void MediaRouterDesktopImpl::DoStartObservingMediaRoutes(
    const MediaSource::Id& source_id) {
  if (event_page_request_manager_->mojo_connections_ready()) {
    MediaRouterMojoImpl::DoStartObservingMediaRoutes(source_id);
    return;
  }
  event_page_request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouterDesktopImpl::DoStartObservingMediaRoutes,
                     weak_factory_.GetWeakPtr(), source_id),
      MediaRouteProviderWakeReason::START_OBSERVING_MEDIA_ROUTES);
}

void MediaRouterDesktopImpl::DoStopObservingMediaRoutes(
    const MediaSource::Id& source_id) {
  if (event_page_request_manager_->mojo_connections_ready()) {
    MediaRouterMojoImpl::DoStopObservingMediaRoutes(source_id);
    return;
  }
  event_page_request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouterDesktopImpl::DoStopObservingMediaRoutes,
                     weak_factory_.GetWeakPtr(), source_id),
      MediaRouteProviderWakeReason::STOP_OBSERVING_MEDIA_ROUTES);
}

void MediaRouterDesktopImpl::DoSearchSinks(
    const MediaSink::Id& sink_id,
    const MediaSource::Id& source_id,
    const std::string& search_input,
    const std::string& domain,
    MediaSinkSearchResponseCallback sink_callback) {
  if (event_page_request_manager_->mojo_connections_ready()) {
    MediaRouterMojoImpl::DoSearchSinks(sink_id, source_id, search_input, domain,
                                       std::move(sink_callback));
    return;
  }
  event_page_request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouterDesktopImpl::DoSearchSinks,
                     weak_factory_.GetWeakPtr(), sink_id, source_id,
                     search_input, domain, std::move(sink_callback)),
      MediaRouteProviderWakeReason::SEARCH_SINKS);
}

void MediaRouterDesktopImpl::DoCreateMediaRouteController(
    const MediaRoute::Id& route_id,
    mojom::MediaControllerRequest mojo_media_controller_request,
    mojom::MediaStatusObserverPtr mojo_observer) {
  if (event_page_request_manager_->mojo_connections_ready()) {
    MediaRouterMojoImpl::DoCreateMediaRouteController(
        route_id, std::move(mojo_media_controller_request),
        std::move(mojo_observer));
    return;
  }
  event_page_request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouterDesktopImpl::DoCreateMediaRouteController,
                     weak_factory_.GetWeakPtr(), route_id,
                     std::move(mojo_media_controller_request),
                     std::move(mojo_observer)),
      MediaRouteProviderWakeReason::CREATE_MEDIA_ROUTE_CONTROLLER);
}

void MediaRouterDesktopImpl::DoProvideSinks(
    const std::string& provider_name,
    std::vector<MediaSinkInternal> sinks) {
  if (event_page_request_manager_->mojo_connections_ready()) {
    MediaRouterMojoImpl::DoProvideSinks(provider_name, std::move(sinks));
    return;
  }
  event_page_request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouterDesktopImpl::DoProvideSinks,
                     weak_factory_.GetWeakPtr(), provider_name,
                     std::move(sinks)),
      MediaRouteProviderWakeReason::PROVIDE_SINKS);
}

void MediaRouterDesktopImpl::DoUpdateMediaSinks(
    const MediaSource::Id& source_id) {
  if (event_page_request_manager_->mojo_connections_ready()) {
    MediaRouterMojoImpl::DoUpdateMediaSinks(source_id);
    return;
  }
  event_page_request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouterDesktopImpl::DoUpdateMediaSinks,
                     weak_factory_.GetWeakPtr(), source_id),
      MediaRouteProviderWakeReason::UPDATE_MEDIA_SINKS);
}

#if defined(OS_WIN)
void MediaRouterDesktopImpl::DoEnsureMdnsDiscoveryEnabled() {
  if (event_page_request_manager_->mojo_connections_ready()) {
    DVLOG_WITH_INSTANCE(1) << "DoEnsureMdnsDiscoveryEnabled";
    if (!is_mdns_enabled_) {
      media_route_provider_->EnableMdnsDiscovery();
      is_mdns_enabled_ = true;
    }
    return;
  }
  event_page_request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouterDesktopImpl::DoEnsureMdnsDiscoveryEnabled,
                     weak_factory_.GetWeakPtr()),
      MediaRouteProviderWakeReason::ENABLE_MDNS_DISCOVERY);
}
#endif

void MediaRouterDesktopImpl::OnConnectionError() {
  event_page_request_manager_->OnMojoConnectionError();
  binding_.reset();
  MediaRouterMojoImpl::OnConnectionError();
}

void MediaRouterDesktopImpl::SyncStateToMediaRouteProvider() {
#if defined(OS_WIN)
  // The MRPM extension already turns on mDNS discovery for platforms other than
  // Windows. It only relies on this signalling from MR on Windows to avoid
  // triggering a firewall prompt out of the context of MR from the user's
  // perspective. This particular call reminds the extension to enable mDNS
  // discovery when it wakes up, has been upgraded, etc.
  if (should_enable_mdns_discovery_) {
    DoEnsureMdnsDiscoveryEnabled();
  }
#endif
  MediaRouterMojoImpl::SyncStateToMediaRouteProvider();
}

MediaRouterDesktopImpl::MediaRouterDesktopImpl(content::BrowserContext* context)
    : MediaRouterMojoImpl(context),
      event_page_request_manager_(
          EventPageRequestManagerFactory::GetApiForBrowserContext(context)),
      weak_factory_(this) {
  DCHECK(event_page_request_manager_);
#if defined(OS_WIN)
  if (check_firewall == FirewallCheck::RUN) {
    CanFirewallUseLocalPorts(
        base::BindOnce(&MediaRouterMojoImpl::OnFirewallCheckComplete,
                       weak_factory_.GetWeakPtr()));
  }
#endif
}

void MediaRouterDesktopImpl::RegisterMediaRouteProvider(
    mojom::MediaRouteProviderPtr media_route_provider_ptr,
    mojom::MediaRouter::RegisterMediaRouteProviderCallback callback) {
  MediaRouterMojoImpl::RegisterMediaRouteProvider(
      std::move(media_route_provider_ptr), std::move(callback));
#if defined(OS_WIN)
  // The MRPM may have been upgraded or otherwise reload such that we could be
  // seeing an MRPM that doesn't know mDNS is enabled, even if we've told a
  // previously registered MRPM it should be enabled. Furthermore, there may be
  // a pending request to enable mDNS, so don't clear this flag after
  // ExecutePendingRequests().
  is_mdns_enabled_ = false;
#endif
  event_page_request_manager_->OnMojoConnectionsReady();
}

void MediaRouterDesktopImpl::BindToMojoRequest(
    mojo::InterfaceRequest<mojom::MediaRouter> request,
    const extensions::Extension& extension) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  binding_ = base::MakeUnique<mojo::Binding<mojom::MediaRouter>>(
      this, std::move(request));
  binding_->set_connection_error_handler(base::BindOnce(
      &MediaRouterDesktopImpl::OnConnectionError, base::Unretained(this)));

  event_page_request_manager_->SetExtensionId(extension.id());
  if (!provider_version_was_recorded_) {
    MediaRouterMojoMetrics::RecordMediaRouteProviderVersion(extension);
    provider_version_was_recorded_ = true;
  }
}

#if defined(OS_WIN)
void MediaRouterDesktopImpl::EnsureMdnsDiscoveryEnabled() {
  if (is_mdns_enabled_)
    return;

  DoEnsureMdnsDiscoveryEnabled();
  should_enable_mdns_discovery_ = true;
}

void MediaRouterDesktopImpl::OnFirewallCheckComplete(
    bool firewall_can_use_local_ports) {
  if (firewall_can_use_local_ports)
    EnsureMdnsDiscoveryEnabled();
}
#endif

}  // namespace media_router
