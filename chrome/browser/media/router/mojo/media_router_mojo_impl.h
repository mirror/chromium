// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTER_MOJO_IMPL_H_
#define CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTER_MOJO_IMPL_H_

#include <stdint.h>

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

// MediaRouter implementation that delegates calls to a MediaRouteProvider.
class MediaRouterMojoImpl : public MediaRouterBase,
                            public mojom::MediaRouter {
 public:
  using ProviderName = std::string;

  ~MediaRouterMojoImpl() override;

  // MediaRouter implementation.
  void CreateRoute(const MediaSource::Id& source_id,
                   const MediaSink::Id& sink_id,
                   const url::Origin& origin,
                   content::WebContents* web_contents,
                   std::vector<MediaRouteResponseCallback> callbacks,
                   base::TimeDelta timeout,
                   bool incognito) final;
  void JoinRoute(const MediaSource::Id& source_id,
                 const std::string& presentation_id,
                 const url::Origin& origin,
                 content::WebContents* web_contents,
                 std::vector<MediaRouteResponseCallback> callbacks,
                 base::TimeDelta timeout,
                 bool incognito) final;
  void ConnectRouteByRouteId(const MediaSource::Id& source,
                             const MediaRoute::Id& route_id,
                             const url::Origin& origin,
                             content::WebContents* web_contents,
                             std::vector<MediaRouteResponseCallback> callbacks,
                             base::TimeDelta timeout,
                             bool incognito) final;
  void TerminateRoute(const MediaRoute::Id& route_id) final;
  void DetachRoute(const MediaRoute::Id& route_id) final;
  void SendRouteMessage(const MediaRoute::Id& route_id,
                        const std::string& message,
                        SendRouteMessageCallback callback) final;
  void SendRouteBinaryMessage(const MediaRoute::Id& route_id,
                              std::unique_ptr<std::vector<uint8_t>> data,
                              SendRouteMessageCallback callback) final;
  void OnUserGesture() override;
  void SearchSinks(const MediaSink::Id& sink_id,
                   const MediaSource::Id& source_id,
                   const std::string& search_input,
                   const std::string& domain,
                   MediaSinkSearchResponseCallback sink_callback) final;
  void ProvideSinks(const std::string& provider_name,
                    std::vector<MediaSinkInternal> sinks) final;
  scoped_refptr<MediaRouteController> GetRouteController(
      const MediaRoute::Id& route_id) final;
  void RegisterMediaRouteProvider(
      const std::string& provider_name,
      mojom::MediaRouteProviderPtr media_route_provider_ptr,
      mojom::MediaRouter::RegisterMediaRouteProviderCallback callback) override;

  // Issues 0+ calls to |media_route_provider_| to ensure its state is in sync
  // with MediaRouter on a best-effort basis.
  virtual void SyncStateToMediaRouteProvider();

  const std::string& instance_id() const { return instance_id_; }

  void set_instance_id_for_test(const std::string& instance_id) {
    instance_id_ = instance_id;
  }

 protected:
  // Standard constructor, used by
  // MediaRouterMojoImplFactory::GetApiForBrowserContext.
  explicit MediaRouterMojoImpl(content::BrowserContext* context);

  // Requests MRPM to update media sinks.  This allows MRPs that only do
  // discovery on sink queries an opportunity to update discovery results
  // even if the MRP SinkAvailability is marked UNAVAILABLE.
  void UpdateMediaSinks(const MediaSource::Id& source_id);

  // Callback called by MRP's CreateMediaRouteController().
  void OnMediaControllerCreated(const MediaRoute::Id& route_id, bool success);

  // Creates a binding between |this| and |request|. If such a binding was
  // previously made for the same |provider_name|, that is invalidated.
  void BindToMojoRequest(const ProviderName& provider_name,
                         mojo::InterfaceRequest<mojom::MediaRouter> request);

  // Methods for obtaining the name of the provider associated with the given
  // object. They return an empty string when such a provider is not found.
  ProviderName GetProviderForRoute(const MediaRoute::Id& route_id) const;
  ProviderName GetProviderForSink(const MediaSink::Id& sink_id,
                                  const MediaSource::Id& source_id) const;
  virtual ProviderName GetProviderForPresentation(
      const std::string& presentation_id) const;

  // Gets the name that a provider is registered under in
  // |media_route_providers_|. E.g. the registered name for "cast" or "dial"
  // provider may be "extension".
  virtual ProviderName GetCanonicalProvider(
      const ProviderName& provider_name) const;

  content::BrowserContext* context() const { return context_; }

  // Mojo pointers to media route providers. Providers are added via
  // RegisterMediaRouteProvider().
  std::unordered_map<ProviderName, mojom::MediaRouteProviderPtr>
      media_route_providers_;

  // Stores route controllers that can be used to send media commands.
  std::unordered_map<MediaRoute::Id, MediaRouteController*> route_controllers_;

 private:
  friend class MediaRouterFactory;
  friend class MediaRouterMojoImplTest;
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
                           RegisterMediaRoutesObserver_DedupingWithCache);
  FRIEND_TEST_ALL_PREFIXES(MediaRouterMojoImplTest,
                           PresentationConnectionStateChangedCallback);
  FRIEND_TEST_ALL_PREFIXES(MediaRouterMojoImplTest,
                           PresentationConnectionStateChangedCallbackRemoved);
  FRIEND_TEST_ALL_PREFIXES(MediaRouterDesktopTest,
                           SyncStateToMediaRouteProvider);
  FRIEND_TEST_ALL_PREFIXES(ExtensionMediaRouteProviderProxyTest,
                           StartAndStopObservingMediaSinks);

  // Represents a query to the MRPs for media sinks and holds observers for the
  // query.
  struct MediaSinksQuery {
   public:
    MediaSinksQuery();
    ~MediaSinksQuery();

    // Caches the list of sinks for the provider returned from the query.
    void CacheSinksForProvider(const ProviderName& provider_name,
                               std::vector<MediaSink> sinks);

    // Clears the cache for all the providers.
    void ClearCachedSinks();

    const std::vector<MediaSink>& cached_sink_list() const {
      return cached_sink_list_;
    }
    const std::unordered_map<ProviderName, std::vector<MediaSink>>
    providers_to_sinks() const {
      return providers_to_sinks_;
    }

    // Cached list of origins for the query.
    std::vector<url::Origin> origins;

    // True if the query has been sent to the MRPs.
    bool is_active = false;

    base::ObserverList<MediaSinksObserver> observers;

   private:
    // Cached list of sinks for the query.
    std::vector<MediaSink> cached_sink_list_;

    // Lists of sinks for each MRP.
    std::unordered_map<ProviderName, std::vector<MediaSink>>
        providers_to_sinks_;

    DISALLOW_COPY_AND_ASSIGN(MediaSinksQuery);
  };

  struct MediaRoutesQuery {
   public:
    MediaRoutesQuery();
    ~MediaRoutesQuery();

    // Caches the list of routes and joinable route IDs for the provider
    // returned from the query.
    void CacheRoutesForProvider(const ProviderName& provider_name,
                                std::vector<MediaRoute> routes,
                                std::vector<MediaRoute::Id> joinable_route_ids);

    // Clears the cache for all the providers.
    void ClearCachedRoutes();

    const base::Optional<std::vector<MediaRoute>>& cached_route_list() const {
      return cached_route_list_;
    }
    const std::vector<MediaRoute::Id>& joinable_route_ids() const {
      return joinable_route_ids_;
    }
    const std::unordered_map<ProviderName, std::vector<MediaRoute>>&
    providers_to_routes() const {
      return providers_to_routes_;
    }

    base::ObserverList<MediaRoutesObserver> observers;

   private:
    // Cached list of routes and joinable route IDs for the query.
    base::Optional<std::vector<MediaRoute>> cached_route_list_;
    std::vector<MediaRoute::Id> joinable_route_ids_;

    // Per-MRP lists of routes and joinable route IDs for the query.
    std::unordered_map<ProviderName, std::vector<MediaRoute>>
        providers_to_routes_;
    std::unordered_map<ProviderName, std::vector<MediaRoute::Id>>
        providers_to_joinable_routes_;

    DISALLOW_COPY_AND_ASSIGN(MediaRoutesQuery);
  };

  class ProviderSinkAvailability {
   public:
    ProviderSinkAvailability();
    virtual ~ProviderSinkAvailability();

    // Sets the sink availability for |provider_name|.
    bool SetForProvider(const ProviderName& provider_name,
                        SinkAvailability availability);

    // Returns true if there is a provider whose sink availability isn't
    // UNAVAILABLE.
    bool IsAvailable() const;

   private:
    void UpdateOverallAvailability();

    std::unordered_map<ProviderName, SinkAvailability> availabilities_;
    SinkAvailability overall_availability_ = SinkAvailability::UNAVAILABLE;
  };

  // MediaRouter implementation.
  bool RegisterMediaSinksObserver(MediaSinksObserver* observer) override;
  void UnregisterMediaSinksObserver(MediaSinksObserver* observer) override;
  void RegisterMediaRoutesObserver(MediaRoutesObserver* observer) override;
  void UnregisterMediaRoutesObserver(MediaRoutesObserver* observer) override;
  void RegisterRouteMessageObserver(RouteMessageObserver* observer) override;
  void UnregisterRouteMessageObserver(RouteMessageObserver* observer) override;
  void DetachRouteController(const MediaRoute::Id& route_id,
                             MediaRouteController* controller) override;

  // Notifies |observer| of any existing cached routes, if it is still
  // registered.
  void NotifyOfExistingRoutesIfRegistered(const MediaSource::Id& source_id,
                                          MediaRoutesObserver* observer) const;

  // mojom::MediaRouter implementation.
  void OnIssue(const IssueInfo& issue) override;
  void OnSinksReceived(const std::string& provider_name,
                       const std::string& media_source,
                       const std::vector<MediaSinkInternal>& internal_sinks,
                       const std::vector<url::Origin>& origins) override;
  void OnRoutesUpdated(
      const std::string& provider_name,
      const std::vector<MediaRoute>& routes,
      const std::string& media_source,
      const std::vector<std::string>& joinable_route_ids) override;
  void OnSinkAvailabilityUpdated(const std::string& provider_name,
                                 SinkAvailability availability) override;
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

  // Invalidates and removes controllers from |route_controllers_| whose media
  // routes do not appear in |routes|.
  void RemoveInvalidRouteControllers(const std::vector<MediaRoute>& routes);

  // Called when the Mojo pointer for |provider_name| is invalidated. Removes
  // the pointer from |media_route_providers_|.
  void OnProviderInvalidated(const ProviderName& provider_name);

  // Called when the binding for |provider_name| in |bindings_| is invalidated.
  // Removes the binding from the map.
  void OnBindingInvalidated(const ProviderName& provider_name);

  std::unordered_map<MediaSource::Id, std::unique_ptr<MediaSinksQuery>>
      sinks_queries_;

  std::unordered_map<MediaSource::Id, std::unique_ptr<MediaRoutesQuery>>
      routes_queries_;

  std::unordered_map<MediaRoute::Id,
                     std::unique_ptr<base::ObserverList<RouteMessageObserver>>>
      message_observers_;

  // GUID unique to each browser run. Component extension uses this to detect
  // when its persisted state was written by an older browser instance, and is
  // therefore stale.
  std::string instance_id_;

  // The last reported sink availability from the media route providers.
  ProviderSinkAvailability availability_;

  // Bindings for Mojo pointers to |this| held by media route providers.
  std::unordered_map<ProviderName,
                     std::unique_ptr<mojo::Binding<mojom::MediaRouter>>>
      bindings_;

  // Mojo pointers to media route providers.
  std::unordered_map<ProviderName, mojom::MediaRouteProviderPtr> providers_;

  content::BrowserContext* const context_;

  base::WeakPtrFactory<MediaRouterMojoImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaRouterMojoImpl);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTER_MOJO_IMPL_H_
