// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_MOJO_LOCAL_MEDIA_ROUTE_PROVIDER_H_
#define CHROME_BROWSER_MEDIA_ROUTER_MOJO_LOCAL_MEDIA_ROUTE_PROVIDER_H_

#include "base/memory/weak_ptr.h"
#include "chrome/common/media_router/mojo/media_router.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

// TODO: remove Browser
class Browser;
class Profile;

namespace content {
class WebContents;
}

namespace media_router {

// A class that forwards MediaRouteProvider calls to the implementation in the
// component extension via EventPageRequestManager.
//
// The MediaRouter implementation holds a Mojo pointer bound to this object via
// the request passed into the ctor.
//
// Calls from this object to the component extension MRP are queued with
// EventPageRequestManager. When the extension is awake, the Mojo pointer to the
// MRP is valid, so the request is executed immediately. Otherwise, when the
// extension is awakened, this object obtains a valid Mojo pointer to the MRP
// from MediaRouter, and MediaRouter makes EventPageRequestManager execute all
// the requests.
class LocalMediaRouteProvider : public mojom::MediaRouteProvider {
 public:
  static const char kProviderName[];

  LocalMediaRouteProvider(mojom::MediaRouteProviderRequest request,
                          mojom::MediaRouterPtr media_router,
                          Profile* profile);
  ~LocalMediaRouteProvider() override;

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

 private:
  void CreatePresentation(const std::string& media_source,
                          const std::string& sink_id,
                          const std::string& presentation_id);

  void NotifyRouteObservers() const;

  // Binds |this| to the Mojo request passed into the ctor.
  mojo::Binding<mojom::MediaRouteProvider> binding_;

  mojom::MediaRouterPtr media_router_;

  // TODO: need to use some kind of incognito profile
  Profile* const profile_;

  std::map<MediaRoute::Id, MediaRoute> routes_;

  // Map from route IDs to corresponding presentation IDs. Multiple routes may
  // share a presentation ID.
  std::map<MediaRoute::Id, std::string> routes_to_presentations_;

  // Map from presentation IDs to presentation windows.
  // TODO: use LocalPresentationWindow instead of Browser
  // TODO: unique_ptr is probably not necessary
  std::map<std::string, std::unique_ptr<Browser>> presentations_;
  // TODO: remove or merge with above
  std::map<std::string, content::WebContents*> presentation_web_contents_;

  std::set<std::string> route_queries_;

  std::set<std::string> sink_queries_;

  int route_index_ = 0;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_MOJO_LOCAL_MEDIA_ROUTE_PROVIDER_H_
