// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTE_PROVIDER_MANAGER_H_
#define CHROME_BROWSER_MEDIA_ROUTE_PROVIDER_MANAGER_H_

#include "base/macros.h"
#include "chrome/common/media_router/mojo/media_router.mojom.h"

namespace media_router {

class MediaRouteProviderManager : public mojom::MediaRouteProvider,
                                  public mojom::MediaRouter {
 public:
  MediaRouteProviderManager();
  ~MediaRouteProviderManager() override;

  static char kProviderName[];

  // mojom::MediaRouteProvider:
  void CreateRoute(const std::string& media_source,
                   const std::string& sink_id,
                   const std::string& original_presentation_id,
                   const url::Origin& origin,
                   int32_t tab_id,
                   base::TimeDelta timeout,
                   bool incognito,
                   CreateRouteCallback callback) override;
  void JoinRoute(const std::string& media_source,
                 const std::string& presentation_id,
                 const url::Origin& origin,
                 int32_t tab_id,
                 base::TimeDelta timeout,
                 bool incognito,
                 JoinRouteCallback callback) override;
  void ConnectRouteByRouteId(const std::string& media_source,
                             const std::string& route_id,
                             const std::string& presentation_id,
                             const url::Origin& origin,
                             int32_t tab_id,
                             base::TimeDelta timeout,
                             bool incognito,
                             ConnectRouteByRouteIdCallback callback) override;
  void TerminateRoute(const std::string& route_id,
                      TerminateRouteCallback callback) override;
  void SendRouteMessage(const std::string& media_route_id,
                        const std::string& message,
                        SendRouteMessageCallback callback) override;
  void SendRouteBinaryMessage(const std::string& media_route_id,
                              const std::vector<uint8_t>& data,
                              SendRouteBinaryMessageCallback callback) override;
  void StartObservingMediaSinks(const std::string& media_source) override;
  void StopObservingMediaSinks(const std::string& media_source) override;
  void StartObservingMediaRoutes(const std::string& media_source) override;
  void StopObservingMediaRoutes(const std::string& media_source) override;
  void StartListeningForRouteMessages(const std::string& route_id) override;
  void StopListeningForRouteMessages(const std::string& route_id) override;
  void DetachRoute(const std::string& route_id) override;
  void EnableMdnsDiscovery() override;
  void UpdateMediaSinks(const std::string& media_source) override;
  void SearchSinks(const std::string& sink_id,
                   const std::string& media_source,
                   mojom::SinkSearchCriteriaPtr search_criteria,
                   SearchSinksCallback callback) override;
  void ProvideSinks(
      const std::string& provider_name,
      const std::vector<media_router::MediaSinkInternal>& sinks) override;
  void CreateMediaRouteController(
      const std::string& route_id,
      mojom::MediaControllerRequest media_controller,
      mojom::MediaStatusObserverPtr observer,
      CreateMediaRouteControllerCallback callback) override;

  // mojom::MediaRouter:
  void RegisterMediaRouteProvider(
    const std::string& provider_name,
      mojom::MediaRouteProviderPtr media_router_provider,
      RegisterMediaRouteProviderCallback callback) override;
  void OnSinksReceived(
    const std::string& provider_name,
      const std::string& media_source,
      const std::vector<media_router::MediaSinkInternal>& sinks,
      const std::vector<url::Origin>& origins) override;
  void OnIssue(const media_router::IssueInfo& issue) override;
  void OnRoutesUpdated(
    const std::string& provider_name,
      const std::vector<media_router::MediaRoute>& routes,
      const std::string& media_source,
      const std::vector<std::string>& joinable_route_ids) override;
  void OnSinkAvailabilityUpdated(
    const std::string& provider_name,
      MediaRouter::SinkAvailability availability) override;
  void OnPresentationConnectionStateChanged(
      const std::string& route_id,
      content::PresentationConnectionState state) override;
  void OnPresentationConnectionClosed(
      const std::string& route_id,
      content::PresentationConnectionCloseReason reason,
      const std::string& message) override;
  void OnRouteMessagesReceived(
      const std::string& route_id,
      const std::vector<content::PresentationConnectionMessage>& messages)
      override;
  void OnMediaRemoterCreated(int32_t tab_id,
                             ::media::mojom::MirrorServiceRemoterPtr remoter,
                             ::media::mojom::MirrorServiceRemotingSourceRequest
                                 remoting_source) override;

 private:
  std::string GetProviderForRoute(const std::string& route_id) const;
  std::string GetProviderForSink(const std::string& sink_id) const;
  std::string GetProviderForPresentation(
      const std::string& presentation_id) const;

  std::vector<MediaSinkInternal> GetSinksForSource(
      const std::string& media_source) const;

  std::unordered_map<std::string, mojom::MediaRouteProviderPtr> providers_;
  std::unordered_map<std::string, std::string> route_id_to_provider_name_;
  std::unordered_map<std::string, std::string> sink_id_to_provider_name_;

  std::unordered_map<std::string, SinkAvailability> sink_availabilities_;
  std::unordered_map<
      std::string,
      std::unordered_map<std::string, std::vector<MediaSinkInternal>>>
      sinks_;
  std::unordered_map<std::string,
                     std::unordered_map<std::string, std::vector<MediaRoute>>>
      routes_;
  std::unordered_map<std::string,
                     std::unordered_map<std::string, std::vector<std::string>>>
      joinable_routes_;

  MediaRouter* const media_router_ = 0;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTE_PROVIDER_MANAGER_H_
