// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/media_route_provider_manager.h"

namespace media_router {

MediaRouteProviderManager::MediaRouteProviderManager() {}
MediaRouteProviderManager::~MediaRouteProviderManager() {}

// static
char MediaRouteProviderManager::kProviderName[] = "manager";

void MediaRouteProviderManager::CreateRoute(
    const std::string& media_source,
    const std::string& sink_id,
    const std::string& original_presentation_id,
    const url::Origin& origin,
    int32_t tab_id,
    base::TimeDelta timeout,
    bool incognito,
    CreateRouteCallback callback) {
  std::string provider_name = GetProviderForSink(sink_id);
  if (!provider_name.empty()) {
    providers_.at(provider_name)
        ->CreateRoute(media_source, sink_id, original_presentation_id, origin,
                      tab_id, timeout, incognito, std::move(callback));
  }
}

void MediaRouteProviderManager::JoinRoute(const std::string& media_source,
                                          const std::string& presentation_id,
                                          const url::Origin& origin,
                                          int32_t tab_id,
                                          base::TimeDelta timeout,
                                          bool incognito,
                                          JoinRouteCallback callback) {
  std::string provider_name = GetProviderForPresentation(presentation_id);
  if (!provider_name.empty()) {
    providers_.at(provider_name)
        ->JoinRoute(media_source, presentation_id, origin, tab_id, timeout,
                    incognito, std::move(callback));
  }
}

void MediaRouteProviderManager::ConnectRouteByRouteId(
    const std::string& media_source,
    const std::string& route_id,
    const std::string& presentation_id,
    const url::Origin& origin,
    int32_t tab_id,
    base::TimeDelta timeout,
    bool incognito,
    ConnectRouteByRouteIdCallback callback) {
  std::string provider_name = GetProviderForRoute(route_id);
  if (!provider_name.empty()) {
    providers_.at(provider_name)
        ->ConnectRouteByRouteId(media_source, route_id, presentation_id, origin,
                                tab_id, timeout, incognito,
                                std::move(callback));
  }
}

void MediaRouteProviderManager::TerminateRoute(
    const std::string& route_id,
    TerminateRouteCallback callback) {
  std::string provider_name = GetProviderForRoute(route_id);
  if (!provider_name.empty()) {
    providers_.at(provider_name)->TerminateRoute(route_id, std::move(callback));
  }
}

void MediaRouteProviderManager::SendRouteMessage(
    const std::string& route_id,
    const std::string& message,
    SendRouteMessageCallback callback) {
  std::string provider_name = GetProviderForRoute(route_id);
  if (!provider_name.empty()) {
    providers_.at(provider_name)
        ->SendRouteMessage(route_id, message, std::move(callback));
  }
}

void MediaRouteProviderManager::SendRouteBinaryMessage(
    const std::string& route_id,
    const std::vector<uint8_t>& data,
    SendRouteBinaryMessageCallback callback) {
  std::string provider_name = GetProviderForRoute(route_id);
  if (!provider_name.empty()) {
    providers_.at(provider_name)
        ->SendRouteBinaryMessage(route_id, data, std::move(callback));
  }
}

void MediaRouteProviderManager::StartObservingMediaSinks(
    const std::string& media_source) {
  for (const auto& provider : providers_)
    provider.second->StartObservingMediaSinks(media_source);
}

void MediaRouteProviderManager::StopObservingMediaSinks(
    const std::string& media_source) {
  for (const auto& provider : providers_)
    provider.second->StopObservingMediaSinks(media_source);
}

void MediaRouteProviderManager::StartObservingMediaRoutes(
    const std::string& media_source) {
  for (const auto& provider : providers_)
    provider.second->StartObservingMediaRoutes(media_source);
}

void MediaRouteProviderManager::StopObservingMediaRoutes(
    const std::string& media_source) {
  for (const auto& provider : providers_)
    provider.second->StopObservingMediaRoutes(media_source);
}

void MediaRouteProviderManager::StartListeningForRouteMessages(
    const std::string& route_id) {
  std::string provider_name = GetProviderForRoute(route_id);
  if (!provider_name.empty()) {
    providers_.at(provider_name)->StartListeningForRouteMessages(route_id);
  }
}

void MediaRouteProviderManager::StopListeningForRouteMessages(
    const std::string& route_id) {
  std::string provider_name = GetProviderForRoute(route_id);
  if (!provider_name.empty()) {
    providers_.at(provider_name)->StopListeningForRouteMessages(route_id);
  }
}

void MediaRouteProviderManager::DetachRoute(const std::string& route_id) {
  std::string provider_name = GetProviderForRoute(route_id);
  if (!provider_name.empty()) {
    providers_.at(provider_name)->DetachRoute(route_id);
  }
}

void MediaRouteProviderManager::EnableMdnsDiscovery() {
  for (const auto& provider : providers_)
    provider.second->EnableMdnsDiscovery();
}

void MediaRouteProviderManager::UpdateMediaSinks(
    const std::string& media_source) {
  for (const auto& provider : providers_)
    provider.second->UpdateMediaSinks(media_source);
}

void MediaRouteProviderManager::SearchSinks(
    const std::string& sink_id,
    const std::string& media_source,
    mojom::SinkSearchCriteriaPtr search_criteria,
    SearchSinksCallback callback) {
  std::string provider_name = GetProviderForSink(sink_id);
  if (!provider_name.empty()) {
    providers_.at(provider_name)
        ->SearchSinks(sink_id, media_source, std::move(search_criteria),
                      std::move(callback));
  }
}

void MediaRouteProviderManager::ProvideSinks(
    const std::string& provider_name,
    const std::vector<media_router::MediaSinkInternal>& sinks) {
  auto provider = providers_.find(provider_name);
  if (provider != providers_.end())
    provider->second->ProvideSinks(provider_name, sinks);
}

void MediaRouteProviderManager::CreateMediaRouteController(
    const std::string& route_id,
    mojom::MediaControllerRequest media_controller,
    mojom::MediaStatusObserverPtr observer,
    CreateMediaRouteControllerCallback callback) {
  std::string provider_name = GetProviderForRoute(route_id);
  if (!provider_name.empty()) {
    providers_.at(provider_name)
        ->CreateMediaRouteController(route_id, std::move(media_controller),
                                     std::move(observer), std::move(callback));
  }
}

void MediaRouteProviderManager::RegisterMediaRouteProvider(
    const std::string& provider_name,
    mojom::MediaRouteProviderPtr media_route_provider,
    RegisterMediaRouteProviderCallback callback) {
  providers_[provider_name] = std::move(media_route_provider);
  // std::move(callback).Run();
}

void MediaRouteProviderManager::OnSinksReceived(
    const std::string& provider_name,
    const std::string& media_source,
    const std::vector<media_router::MediaSinkInternal>& sinks,
    const std::vector<url::Origin>& origins) {
  sinks_[media_source][provider_name] = sinks;
  // media_router_->OnSinksReceived(kProviderName, media_source,
  //                                GetSinksForSource(media_source), origins);
}

void MediaRouteProviderManager::OnIssue(const media_router::IssueInfo& issue) {
  media_router_->OnIssue(issue);
}

void MediaRouteProviderManager::OnRoutesUpdated(
    const std::string& provider_name,
    const std::vector<media_router::MediaRoute>& routes,
    const std::string& media_source,
    const std::vector<std::string>& joinable_route_ids) {
  routes_[media_source][provider_name] = routes;
  joinable_routes_[media_source][provider_name] = joinable_route_ids;
  // media_router_->OnRoutesUpdated(
  //     kProviderName, GetRoutesForSource(media_source), media_source,
  //     GetJoinableRouteIdsForSource(joinable_route_ids));
}

void MediaRouteProviderManager::OnSinkAvailabilityUpdated(
    const std::string& provider_name,
    MediaRouter::SinkAvailability availability) {
  sink_availabilities_[provider_name] = availability;
  // media_router_->OnSinkAvailabilityUpdated(kProviderName,
  //                                          GetOverallSinkAvailability());
}

void MediaRouteProviderManager::OnPresentationConnectionStateChanged(
    const std::string& route_id,
    content::PresentationConnectionState state) {
  media_router_->OnPresentationConnectionStateChanged(route_id, state);
}

void MediaRouteProviderManager::OnPresentationConnectionClosed(
    const std::string& route_id,
    content::PresentationConnectionCloseReason reason,
    const std::string& message) {
  media_router_->OnPresentationConnectionClosed(route_id, reason, message);
}

void MediaRouteProviderManager::OnRouteMessagesReceived(
    const std::string& route_id,
    const std::vector<content::PresentationConnectionMessage>& messages) {
  media_router_->OnRouteMessagesReceived(route_id, messages);
}

void MediaRouteProviderManager::OnMediaRemoterCreated(
    int32_t tab_id,
    media::mojom::MirrorServiceRemoterPtr remoter,
    media::mojom::MirrorServiceRemotingSourceRequest remoting_source) {
  media_router_->OnMediaRemoterCreated(tab_id, std::move(remoter),
                                       std::move(remoting_source));
}

std::string MediaRouteProviderManager::GetProviderForRoute(
    const std::string& route_id) const {
  auto provider = route_id_to_provider_name_.find(route_id);
  if (provider != route_id_to_provider_name_.end()) {
    return provider->second;
  } else {
    return std::string();
  }
}

std::string MediaRouteProviderManager::GetProviderForSink(
    const std::string& sink_id) const {
  auto provider = sink_id_to_provider_name_.find(sink_id);
  if (provider != sink_id_to_provider_name_.end()) {
    return provider->second;
  } else {
    return std::string();
  }
}

std::string MediaRouteProviderManager::GetProviderForPresentation(
    const std::string& presentation_id) const {
  return std::string();
}

std::vector<MediaSinkInternal> MediaRouteProviderManager::GetSinksForSource(
    const std::string& media_source) const {
  return {};
}

}  // namespace media_router
