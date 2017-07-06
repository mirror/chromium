// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTE_PROVIDER_PROXY_H
#define CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTE_PROVIDER_PROXY_H

#include "base/memory/weak_ptr.h"
#include "chrome/browser/media/router/mojo/media_router_mojo_impl.h"
#include "chrome/common/media_router/mojo/media_router.mojom.h"

namespace content {
class BrowserContext;
}

namespace media_router {

class EventPageRequestManager;

class MediaRouteProviderProxy : public mojom::MediaRouteProvider {
 public:
  MediaRouteProviderProxy(content::BrowserContext* context,
                          MediaRouterMojoImpl* media_router);
  ~MediaRouteProviderProxy() override;

  void RegisterMediaRouteProvider(
      mojom::MediaRouteProviderPtr media_route_provider_ptr,
      mojom::MediaRouter::RegisterMediaRouteProviderCallback callback);

  // mojom::MediaRouteProvider implementation.
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

 private:
  void OnConnectionError();

  void DoCreateRoute(const std::string& media_source,
                     const std::string& sink_id,
                     const std::string& original_presentation_id,
                     const url::Origin& origin,
                     int32_t tab_id,
                     base::TimeDelta timeout,
                     bool incognito,
                     CreateRouteCallback callback);
  void DoJoinRoute(const std::string& media_source,
                   const std::string& presentation_id,
                   const url::Origin& origin,
                   int32_t tab_id,
                   base::TimeDelta timeout,
                   bool incognito,
                   JoinRouteCallback callback);
  void DoConnectRouteByRouteId(const std::string& media_source,
                               const std::string& route_id,
                               const std::string& presentation_id,
                               const url::Origin& origin,
                               int32_t tab_id,
                               base::TimeDelta timeout,
                               bool incognito,
                               ConnectRouteByRouteIdCallback callback);
  void DoTerminateRoute(const std::string& route_id,
                        TerminateRouteCallback callback);
  void DoSendRouteMessage(const std::string& media_route_id,
                          const std::string& message,
                          SendRouteMessageCallback callback);
  void DoSendRouteBinaryMessage(const std::string& media_route_id,
                                const std::vector<uint8_t>& data,
                                SendRouteBinaryMessageCallback callback);
  void DoStartObservingMediaSinks(const std::string& media_source);
  void DoStopObservingMediaSinks(const std::string& media_source);
  void DoStartObservingMediaRoutes(const std::string& media_source);
  void DoStopObservingMediaRoutes(const std::string& media_source);
  void DoStartListeningForRouteMessages(const std::string& route_id);
  void DoStopListeningForRouteMessages(const std::string& route_id);
  void DoDetachRoute(const std::string& route_id);
  void DoEnableMdnsDiscovery();
  void DoUpdateMediaSinks(const std::string& media_source);
  void DoSearchSinks(const std::string& sink_id,
                     const std::string& media_source,
                     mojom::SinkSearchCriteriaPtr search_criteria,
                     SearchSinksCallback callback);
  void DoProvideSinks(
      const std::string& provider_name,
      const std::vector<media_router::MediaSinkInternal>& sinks);
  void DoCreateMediaRouteController(
      const std::string& route_id,
      mojom::MediaControllerRequest media_controller,
      mojom::MediaStatusObserverPtr observer,
      CreateMediaRouteControllerCallback callback);

  // Mojo proxy object for the Media Route Provider Manager.
  // Set to null initially, and later set to the Provider Manager Mojo pointer
  // passed in via |RegisterMediaRouteProvider()|.
  // This is set to null again when the component extension is suspended
  // if or a Mojo channel error occured.
  mojom::MediaRouteProviderPtr media_route_provider_;

  // Request manager responsible for waking the component extension and calling
  // the requests to it.
  EventPageRequestManager* const request_manager_;

  MediaRouterMojoImpl* const media_router_;

  base::WeakPtrFactory<MediaRouteProviderProxy> weak_factory_;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTE_PROVIDER_PROXY_H
