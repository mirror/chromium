// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_MOJO_wired_display_media_route_provider_H_
#define CHROME_BROWSER_MEDIA_ROUTER_MOJO_wired_display_media_route_provider_H_

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/media/router/offscreen_presentation_manager.h"
#include "chrome/common/media_router/mojo/media_router.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "ui/display/display_observer.h"

// TODO: remove Browser
class Browser;
class Profile;

namespace content {
class WebContents;
}

namespace media_router {

// A MediaRouteProvider class that provides wired displays as media sinks.
// A display can be used as a sink if it is external and does not have a browser
// window with a pending presentation controller (i.e. a Media Router dialog).
class WiredDisplayMediaRouteProvider
    : public mojom::MediaRouteProvider,
      public display::DisplayObserver,
      public OffscreenPresentationManager::Observer {
 public:
  static const Id kProviderId;

  WiredDisplayMediaRouteProvider(mojom::MediaRouteProviderRequest request,
                                 mojom::MediaRouterPtr media_router,
                                 Profile* profile);
  ~WiredDisplayMediaRouteProvider() override;

  // mojom::MediaRouteProvider:
  void CreateRoute(const std::string& media_source,
                   const std::string& sink_id,
                   const std::string& presentation_id,
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

  // display::DisplayObserver:
  void OnDisplayAdded(const display::Display& new_display) override;
  void OnDisplayRemoved(const display::Display& old_display) override;

  // OffscreenPresentationManager::Observer:
  void OnPendingControllersUpdated() override;

 private:
  void CreatePresentation(const std::string& media_source,
                          const std::string& sink_id,
                          const std::string& presentation_id);

  // Send the current list of routes to each query in |route_queries_|.
  void NotifyRouteObservers() const;

  // Send the current list of sinks to each query in |sink_queries_|.
  void NotifySinkObservers() const;

  // Returns a list of available sinks. A display can be a sink if it is
  // external and does not contain a page that is about to be a presentation
  // controller.
  std::vector<MediaSinkInternal> GetSinks() const;

  // Binds |this| to the Mojo request passed into the ctor.
  mojo::Binding<mojom::MediaRouteProvider> binding_;

  // Mojo pointer to the Media Router.
  mojom::MediaRouterPtr media_router_;

  // TODO: need to use some kind of incognito profile
  Profile* const profile_;

  // Active routes managed by this provider.
  base::flat_map<MediaRoute::Id, MediaRoute> routes_;

  // Map from route IDs to corresponding presentation IDs.
  base::flat_map<MediaRoute::Id, std::string> routes_to_presentations_;

  // Map from presentation IDs to presentation windows.
  // TODO: use LocalPresentationWindow instead of Browser
  // TODO: unique_ptr is probably not necessary
  base::flat_map<std::string, std::unique_ptr<Browser>> presentations_;
  // TODO: remove
  base::flat_map<std::string, content::WebContents*> presentation_web_contents_;

  // A set of MediaSource IDs associated with queries for MediaRoute updates.
  base::flat_set<std::string> route_queries_;

  // A set of MediaSource IDs associated with queries for MediaSink updates.
  base::flat_set<std::string> sink_queries_;

  // A non-decreasing counter used to generate MediaRoute IDs.
  int route_index_ = 0;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_MOJO_wired_display_media_route_provider_H_
