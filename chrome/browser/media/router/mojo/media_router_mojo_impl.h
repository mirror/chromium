// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTER_MOJO_IMPL_H_
#define CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTER_MOJO_IMPL_H_

#include <stdint.h>

#include <deque>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/containers/hash_tables.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/optional.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "chrome/browser/media/router/issue_manager.h"
#include "chrome/browser/media/router/media_router_base.h"
#include "chrome/browser/media/router/media_routes_observer.h"
#include "chrome/common/media_router/issue.h"
#include "chrome/common/media_router/mojo/media_router.mojom.h"
#include "chrome/common/media_router/route_request_result.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {
class BrowserContext;
}

namespace media_router {

enum class MediaRouteProviderWakeReason;
class CastMediaSinkService;
class DialMediaSinkServiceProxy;

// MediaRouter implementation that delegates calls to a MediaRouteProvider.
// Lives on the UI thread.
class MediaRouterMojoImpl : public MediaRouterBase,
                            public mojom::MediaRouter {
 public:
  ~MediaRouterMojoImpl() override;

  // MediaRouter implementation.
  // Execution of the requests is delegated to the Do* methods, which can be
  // enqueued for later use if the extension is temporarily suspended.
  void CreateRoute(const MediaSource::Id& source_id,
                   const MediaSink::Id& sink_id,
                   const url::Origin& origin,
                   content::WebContents* web_contents,
                   std::vector<MediaRouteResponseCallback> callbacks,
                   base::TimeDelta timeout,
                   bool incognito) override;
  void JoinRoute(const MediaSource::Id& source_id,
                 const std::string& presentation_id,
                 const url::Origin& origin,
                 content::WebContents* web_contents,
                 std::vector<MediaRouteResponseCallback> callbacks,
                 base::TimeDelta timeout,
                 bool incognito) override;
  void ConnectRouteByRouteId(const MediaSource::Id& source,
                             const MediaRoute::Id& route_id,
                             const url::Origin& origin,
                             content::WebContents* web_contents,
                             std::vector<MediaRouteResponseCallback> callbacks,
                             base::TimeDelta timeout,
                             bool incognito) override;
  void TerminateRoute(const MediaRoute::Id& route_id) override;
  void DetachRoute(const MediaRoute::Id& route_id) override;
  void SendRouteMessage(const MediaRoute::Id& route_id,
                        const std::string& message,
                        SendRouteMessageCallback callback) override;
  void SendRouteBinaryMessage(const MediaRoute::Id& route_id,
                              std::unique_ptr<std::vector<uint8_t>> data,
                              SendRouteMessageCallback callback) override;
  void AddIssue(const IssueInfo& issue_info) override;
  void ClearIssue(const Issue::Id& issue_id) override;
  void OnUserGesture() override;
  void SearchSinks(const MediaSink::Id& sink_id,
                   const MediaSource::Id& source_id,
                   const std::string& search_input,
                   const std::string& domain,
                   MediaSinkSearchResponseCallback sink_callback) override;
  void ProvideSinks(const std::string& provider_name,
                    std::vector<MediaSinkInternal> sinks) override;
  scoped_refptr<MediaRouteController> GetRouteController(
      const MediaRoute::Id& route_id) override;

  // These methods return true if there has not been a state change that makes
  // a request to start or stop observing for the given MediaSource no longer
  // necessary.
  bool ShouldStartObservingSinks(const MediaSource::Id& media_source);
  bool ShouldStopObservingMediaSinks(const MediaSource::Id& media_source);
  bool ShouldStartObservingMediaRoutes(const MediaSource::Id& media_source);
  bool ShouldStopObservingMediaRoutes(const MediaSource::Id& media_source);

  // Issues 0+ calls to |media_route_provider_| to ensure its state is in sync
  // with MediaRouter on a best-effort basis.
  // The MediaRouteProvider implementation might have become out of sync with
  // MediaRouter due to one of few reasons:
  // (1) The extension crashed and lost unpersisted changes.
  // (2) The extension was updated; temporary data is cleared.
  // (3) The extension has an unforseen bug which causes temporary data to be
  //     persisted incorrectly on suspension.
  void SyncStateToMediaRouteProvider();

  // Binds |this| to a Mojo interface request, so that clients can acquire a
  // handle to a MediaRouterMojoImpl instance via the Mojo service connector.
  // Stores the ID of |extension| in |media_route_provider_extension_id_|.
  void BindToMojoRequest(mojo::InterfaceRequest<mojom::MediaRouter> request,
                         base::OnceClosure error_handler);

  const std::string& instance_id() const { return instance_id_; }

  void set_media_route_provider_for_test(
      std::unique_ptr<mojom::MediaRouteProvider> media_route_provider) {
    media_route_provider_ = std::move(media_route_provider);
  }

  void set_instance_id_for_test(const std::string& instance_id) {
    instance_id_ = instance_id;
  }

 private:
  friend class MediaRouterFactory;
  friend class MediaRouterMojoTest;
  FRIEND_TEST_ALL_PREFIXES(MediaRouterMojoImplTest, JoinRoute);
  FRIEND_TEST_ALL_PREFIXES(MediaRouterMojoImplTest, JoinRouteTimedOutFails);
  FRIEND_TEST_ALL_PREFIXES(MediaRouterMojoImplTest,
                           JoinRouteIncognitoMismatchFails);
  FRIEND_TEST_ALL_PREFIXES(MediaRouterMojoImplTest,
                           IncognitoRoutesTerminatedOnProfileShutdown);
  FRIEND_TEST_ALL_PREFIXES(MediaRouterMojoImplTest,
                           RegisterAndUnregisterMediaSinksObserver);
  FRIEND_TEST_ALL_PREFIXES(MediaRouterMojoImplTest,
                           RegisterMediaSinksObserverWithAvailabilityChange);
  FRIEND_TEST_ALL_PREFIXES(
      MediaRouterMojoImplTest,
      RegisterAndUnregisterMediaSinksObserverWithAvailabilityChange);
  FRIEND_TEST_ALL_PREFIXES(MediaRouterMojoImplTest,
                           RegisterAndUnregisterMediaRoutesObserver);
  FRIEND_TEST_ALL_PREFIXES(MediaRouterMojoImplTest,
                           RouteMessagesSingleObserver);
  FRIEND_TEST_ALL_PREFIXES(MediaRouterMojoImplTest,
                           RouteMessagesMultipleObservers);
  FRIEND_TEST_ALL_PREFIXES(MediaRouterMojoImplTest, HandleIssue);
  FRIEND_TEST_ALL_PREFIXES(MediaRouterMojoImplTest, GetRouteController);
  FRIEND_TEST_ALL_PREFIXES(MediaRouterMojoImplTest,
                           GetRouteControllerMultipleTimes);
  FRIEND_TEST_ALL_PREFIXES(MediaRouterMojoImplTest,
                           GetRouteControllerAfterInvalidation);
  FRIEND_TEST_ALL_PREFIXES(MediaRouterMojoImplTest,
                           GetRouteControllerAfterRouteInvalidation);
  FRIEND_TEST_ALL_PREFIXES(MediaRouterMojoImplTest,
                           FailToCreateRouteController);
  FRIEND_TEST_ALL_PREFIXES(MediaRouterMojoImplTest,
                           SyncStateToMediaRouteProvider);

  // Represents a query to the MRPM for media sinks and holds observers for the
  // query.
  struct MediaSinksQuery {
   public:
    MediaSinksQuery();
    ~MediaSinksQuery();

    // True if the query has been sent to the MRPM.
    bool is_active = false;

    // Cached list of sinks and origins for the query.
    base::Optional<std::vector<MediaSink>> cached_sink_list;
    std::vector<url::Origin> origins;

    base::ObserverList<MediaSinksObserver> observers;

   private:
    DISALLOW_COPY_AND_ASSIGN(MediaSinksQuery);
  };

  struct MediaRoutesQuery {
   public:
    MediaRoutesQuery();
    ~MediaRoutesQuery();

    // Cached list of routes and joinable route IDs for the query.
    base::Optional<std::vector<MediaRoute>> cached_route_list;
    std::vector<std::string> joinable_route_ids;

    base::ObserverList<MediaRoutesObserver> observers;

   private:
    DISALLOW_COPY_AND_ASSIGN(MediaRoutesQuery);
  };

  enum class FirewallCheck {
    // Skips the firewall check for the benefit of unit tests so they do not
    // have to depend on the system's firewall configuration.
    SKIP_FOR_TESTING,
    // Perform the firewall check (default).
    RUN,
  };

  // Standard constructor, used by
  // MediaRouterMojoImplFactory::GetApiForBrowserContext.
  MediaRouterMojoImpl(content::BrowserContext* context,
                      FirewallCheck check_firewall = FirewallCheck::RUN);

  // MediaRouter implementation.
  bool RegisterMediaSinksObserver(MediaSinksObserver* observer) override;
  void UnregisterMediaSinksObserver(MediaSinksObserver* observer) override;
  void RegisterMediaRoutesObserver(MediaRoutesObserver* observer) override;
  void UnregisterMediaRoutesObserver(MediaRoutesObserver* observer) override;
  void RegisterIssuesObserver(IssuesObserver* observer) override;
  void UnregisterIssuesObserver(IssuesObserver* observer) override;
  void RegisterRouteMessageObserver(RouteMessageObserver* observer) override;
  void UnregisterRouteMessageObserver(RouteMessageObserver* observer) override;
  void DetachRouteController(const MediaRoute::Id& route_id,
                             MediaRouteController* controller) override;

  // Notifies |observer| of any existing cached routes, if it is still
  // registered.
  void NotifyOfExistingRoutesIfRegistered(const MediaSource::Id& source_id,
                                          MediaRoutesObserver* observer) const;

  // Error handler callback for |binding_| and |media_route_provider_|.
  void OnConnectionError();

  // mojom::MediaRouter implementation.
  void RegisterMediaRouteProvider(
      mojom::MediaRouteProviderPtr media_route_provider_ptr,
      mojom::MediaRouter::RegisterMediaRouteProviderCallback callback) override;
  void OnIssue(const IssueInfo& issue) override;
  void OnSinksReceived(const std::string& media_source,
                       const std::vector<MediaSinkInternal>& internal_sinks,
                       const std::vector<url::Origin>& origins) override;
  void OnRoutesUpdated(
      const std::vector<MediaRoute>& routes,
      const std::string& media_source,
      const std::vector<std::string>& joinable_route_ids) override;
  void OnSinkAvailabilityUpdated(
      mojom::MediaRouter::SinkAvailability availability) override;
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
  void OnMediaRemoterCreated(
      int32_t tab_id,
      media::mojom::MirrorServiceRemoterPtr remoter,
      media::mojom::MirrorServiceRemotingSourceRequest source_request) override;

  // Result callback when Mojo terminateRoute is invoked.  |route_id| is bound
  // to the ID of the route that was terminated.
  void OnTerminateRouteResult(const MediaRoute::Id& route_id,
                              const base::Optional<std::string>& error_text,
                              RouteRequestResult::ResultCode result_code);

  // Converts the callback result of calling Mojo CreateRoute()/JoinRoute()
  // into a local callback.
  void RouteResponseReceived(const std::string& presentation_id,
                             bool is_incognito,
                             std::vector<MediaRouteResponseCallback> callbacks,
                             bool is_join,
                             const base::Optional<MediaRoute>& media_route,
                             const base::Optional<std::string>& error_text,
                             RouteRequestResult::ResultCode result_code);

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

  // Start browser side sink discovery.
  void StartDiscovery();

  // Requests MRPM to update media sinks.  This allows MRPs that only do
  // discovery on sink queries an opportunity to update discovery results
  // even if the MRP SinkAvailability is marked UNAVAILABLE.
  void UpdateMediaSinks(const MediaSource::Id& source_id);

  // Invalidates and removes controllers from |route_controllers_| whose media
  // routes do not appear in |routes|.
  void RemoveInvalidRouteControllers(const std::vector<MediaRoute>& routes);

  // Callback called by MRP's CreateMediaRouteController().
  void OnMediaControllerCreated(const MediaRoute::Id& route_id, bool success);

  std::unordered_map<MediaSource::Id, std::unique_ptr<MediaSinksQuery>>
      sinks_queries_;

  std::unordered_map<MediaSource::Id, std::unique_ptr<MediaRoutesQuery>>
      routes_queries_;

  std::unordered_map<MediaRoute::Id,
                     std::unique_ptr<base::ObserverList<RouteMessageObserver>>>
      message_observers_;

  IssueManager issue_manager_;

  // Binds |this| to a Mojo connection stub for mojom::MediaRouter.
  std::unique_ptr<mojo::Binding<mojom::MediaRouter>> binding_;

  // GUID unique to each browser run. Component extension uses this to detect
  // when its persisted state was written by an older browser instance, and is
  // therefore stale.
  std::string instance_id_;

  // MediaRouteProvider implementation that |this| delegates calls to.
  std::unique_ptr<mojom::MediaRouteProvider> media_route_provider_;

  // The last reported sink availability from the media route provider manager.
  mojom::MediaRouter::SinkAvailability availability_;

  // Stores route controllers that can be used to send media commands to the
  // extension.
  std::unordered_map<MediaRoute::Id, MediaRouteController*> route_controllers_;

  // Media sink service for DIAL devices.
  scoped_refptr<DialMediaSinkServiceProxy> dial_media_sink_service_proxy_;

  // Media sink service for CAST devices.
  scoped_refptr<CastMediaSinkService> cast_media_sink_service_;

  content::BrowserContext* const context_;

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

  base::WeakPtrFactory<MediaRouterMojoImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaRouterMojoImpl);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTER_MOJO_IMPL_H_
