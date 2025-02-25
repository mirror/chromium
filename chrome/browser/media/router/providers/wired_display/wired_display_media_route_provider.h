// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_WIRED_DISPLAY_WIRED_DISPLAY_MEDIA_ROUTE_PROVIDER_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_WIRED_DISPLAY_WIRED_DISPLAY_MEDIA_ROUTE_PROVIDER_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "chrome/browser/media/router/discovery/media_sink_discovery_metrics.h"
#include "chrome/common/media_router/media_route_provider_helper.h"
#include "chrome/common/media_router/mojo/media_router.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "ui/display/display.h"
#include "ui/display/display_observer.h"

class Profile;

namespace media_router {

class WiredDisplayPresentationReceiver;

// A MediaRouteProvider class that provides wired displays as media sinks.
// Displays can be used as sinks when there are multiple dipslays that are not
// mirrored.
class WiredDisplayMediaRouteProvider : public mojom::MediaRouteProvider,
                                       public display::DisplayObserver {
 public:
  static const MediaRouteProviderId kProviderId;

  static std::string GetSinkIdForDisplay(const display::Display& display);

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
  void OnDidProcessDisplayChanges() override;
  void OnDisplayAdded(const display::Display& new_display) override;
  void OnDisplayRemoved(const display::Display& old_display) override;
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t changed_metrics) override;

 protected:
  // Returns all displays, including those that may not be used as sinks.
  virtual std::vector<display::Display> GetAllDisplays() const;

  virtual display::Display GetPrimaryDisplay() const;

 private:
  struct Presentation {
   public:
    Presentation(const MediaRoute& route,
                 std::unique_ptr<WiredDisplayPresentationReceiver> receiver);
    Presentation(Presentation&& other);
    ~Presentation();

    MediaRoute route;
    std::unique_ptr<WiredDisplayPresentationReceiver> receiver;

   private:
    DISALLOW_COPY_AND_ASSIGN(Presentation);
  };

  // Sends the current list of routes to each query in |route_queries_|.
  void NotifyRouteObservers() const;

  // Sends the current list of sinks to each query in |sink_queries_|.
  void NotifySinkObservers();

  // Notifies |media_router_| of the current sink availability.
  void ReportSinkAvailability(const std::vector<MediaSinkInternal>& sinks);

  // Removes the presentation from |presentations_| and notifies route
  // observers.
  void RemovePresentationById(const std::string& presentation_id);

  // Updates the description for the route associated with |presentation_id|,
  // and notifies route observers if the description changed.
  void UpdateRouteDescription(const std::string& presentation_id,
                              const std::string& title);

  Presentation CreatePresentation(const std::string& presentation_id,
                                  const display::Display& display,
                                  const MediaRoute& media_route);

  // Terminates all presentation receivers on |display|.
  void TerminatePresentationsOnDisplay(const display::Display& display);

  // Returns a display associated with |sink_id|, or a nullopt if not found.
  base::Optional<display::Display> GetDisplayBySinkId(
      const std::string& sink_id) const;

  // Returns a list of available sinks.
  std::vector<MediaSinkInternal> GetSinks() const;

  // Returns a list of displays that can be used as sinks. Returns an empty list
  // if there is only one display or if the secondary displays mirror the
  // primary display.
  std::vector<display::Display> GetAvailableDisplays() const;

  // Binds |this| to the Mojo request passed into the ctor.
  mojo::Binding<mojom::MediaRouteProvider> binding_;

  // Mojo pointer to the Media Router.
  mojom::MediaRouterPtr media_router_;

  // Presentation profiles are created based on this original profile. This
  // profile is not owned by |this|.
  Profile* profile_;

  // Map from presentation IDs to active presentations managed by this provider.
  std::map<std::string, Presentation> presentations_;

  // A set of MediaSource IDs associated with queries for MediaRoute updates.
  base::flat_set<std::string> route_queries_;

  // A set of MediaSource IDs associated with queries for MediaSink updates.
  base::flat_set<std::string> sink_queries_;

  // Used for recording UMA metrics for the number of sinks available.
  WiredDisplayDeviceCountMetrics device_count_metrics_;

  DISALLOW_COPY_AND_ASSIGN(WiredDisplayMediaRouteProvider);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_WIRED_DISPLAY_WIRED_DISPLAY_MEDIA_ROUTE_PROVIDER_H_
