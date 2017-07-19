// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTER_DESKTOP_IMPL_H_
#define CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTER_DESKTOP_IMPL_H_

#include "build/build_config.h"
#include "chrome/browser/media/router/mojo/media_router_mojo_impl.h"

namespace media_router {

// MediaRouter implementation that delegates calls to the component extension.
// Also handles the suspension and wakeup of the component extension.
// Lives on the UI thread.
class MediaRouterDesktopImpl : public MediaRouterMojoImpl {
 public:
  ~MediaRouterDesktopImpl() override;

  // Sets up the MediaRouterMojoImpl instance owned by |context| to handle
  // MediaRouterObserver requests from the component extension given by
  // |extension|. Creates the MediaRouterMojoImpl instance if it does not
  // exist.
  // Called by the Mojo module registry.
  // |extension|: The component extension, used for querying
  //     suspension state.
  // |context|: The BrowserContext which owns the extension process.
  // |request|: The Mojo connection request used for binding.
  static void BindToRequest(const extensions::Extension* extension,
                            content::BrowserContext* context,
                            const service_manager::BindSourceInfo& source_info,
                            mojom::MediaRouterRequest request);

  void OnUserGesture() override;

 protected:
  void DoCreateRoute(const MediaSource::Id& source_id,
                     const MediaSink::Id& sink_id,
                     const url::Origin& origin,
                     int tab_id,
                     std::vector<MediaRouteResponseCallback> callbacks,
                     base::TimeDelta timeout,
                     bool incognito) override;
  void DoJoinRoute(const MediaSource::Id& source_id,
                   const std::string& presentation_id,
                   const url::Origin& origin,
                   int tab_id,
                   std::vector<MediaRouteResponseCallback> callbacks,
                   base::TimeDelta timeout,
                   bool incognito) override;
  void DoConnectRouteByRouteId(
      const MediaSource::Id& source_id,
      const MediaRoute::Id& route_id,
      const url::Origin& origin,
      int tab_id,
      std::vector<MediaRouteResponseCallback> callbacks,
      base::TimeDelta timeout,
      bool incognito) override;
  void DoTerminateRoute(const MediaRoute::Id& route_id) override;
  void DoDetachRoute(const MediaRoute::Id& route_id) override;
  void DoSendSessionMessage(const MediaRoute::Id& route_id,
                            const std::string& message,
                            SendRouteMessageCallback callback) override;
  void DoSendSessionBinaryMessage(const MediaRoute::Id& route_id,
                                  std::unique_ptr<std::vector<uint8_t>> data,
                                  SendRouteMessageCallback callback) override;
  void DoStartListeningForRouteMessages(
      const MediaRoute::Id& route_id) override;
  void DoStopListeningForRouteMessages(const MediaRoute::Id& route_id) override;
  void DoStartObservingMediaSinks(const MediaSource::Id& source_id) override;
  void DoStopObservingMediaSinks(const MediaSource::Id& source_id) override;
  void DoStartObservingMediaRoutes(const MediaSource::Id& source_id) override;
  void DoStopObservingMediaRoutes(const MediaSource::Id& source_id) override;
  void DoSearchSinks(const MediaSink::Id& sink_id,
                     const MediaSource::Id& source_id,
                     const std::string& search_input,
                     const std::string& domain,
                     MediaSinkSearchResponseCallback sink_callback) override;
  void DoCreateMediaRouteController(
      const MediaRoute::Id& route_id,
      mojom::MediaControllerRequest mojo_media_controller_request,
      mojom::MediaStatusObserverPtr mojo_observer) override;
  void DoProvideSinks(const std::string& provider_name,
                      std::vector<MediaSinkInternal> sinks) override;
  void DoUpdateMediaSinks(const MediaSource::Id& source_id) override;
#if defined(OS_WIN)
  void DoEnsureMdnsDiscoveryEnabled() override;
#endif

  // Error handler callback for |binding_| and |media_route_provider_|.
  void OnConnectionError() override;

  void SyncStateToMediaRouteProvider() override;

 private:
  friend class MediaRouterFactory;
  explicit MediaRouterDesktopImpl(content::BrowserContext* context);

  // mojom::MediaRouter implementation.
  void RegisterMediaRouteProvider(
      mojom::MediaRouteProviderPtr media_route_provider_ptr,
      mojom::MediaRouter::RegisterMediaRouteProviderCallback callback) override;

  // Binds |this| to a Mojo interface request, so that clients can acquire a
  // handle to a MediaRouterMojoImpl instance via the Mojo service connector.
  // Passes the extension's ID to the event page request manager.
  void BindToMojoRequest(mojo::InterfaceRequest<mojom::MediaRouter> request,
                         const extensions::Extension& extension);

#if defined(OS_WIN)
  // Ensures that mDNS discovery is enabled in the MRPM extension. This can be
  // called many times but the MRPM will only be called once per registration
  // period.
  void EnsureMdnsDiscoveryEnabled();

  // Callback used to enabled mDNS in the MRPM if a firewall prompt will not be
  // triggered. If a firewall prompt would be triggered, enabling mDNS won't
  // happen until the user is clearly interacting with MR.
  void OnFirewallCheckComplete(bool firewall_can_use_local_ports);
#endif

  // Request manager responsible for waking the component extension and calling
  // the requests to it.
  EventPageRequestManager* const event_page_request_manager_;

  // Binds |this| to a Mojo connection stub for mojom::MediaRouter.
  std::unique_ptr<mojo::Binding<mojom::MediaRouter>> binding_;

  // A flag to ensure that we record the provider version once, during the
  // initial event page wakeup attempt.
  bool provider_version_was_recorded_ = false;

#if defined(OS_WIN)
  // A pair of flags to ensure that mDNS discovery is only enabled on Windows
  // when there will be appropriate context for the user to associate a firewall
  // prompt with Media Router. |should_enable_mdns_discovery_| can only go from
  // |false| to |true|. On Windows, |is_mdns_enabled_| is set to |false| in
  // RegisterMediaRouteProvider and only set to |true| when we successfully call
  // the extension to enable mDNS.
  bool is_mdns_enabled_ = false;
  bool should_enable_mdns_discovery_ = false;
#endif

  base::WeakPtrFactory<MediaRouterDesktopImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaRouterDesktopImpl);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTER_DESKTOP_IMPL_H_
