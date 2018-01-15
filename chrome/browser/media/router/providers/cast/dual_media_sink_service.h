// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_DUAL_MEDIA_SINK_SERVICE_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_DUAL_MEDIA_SINK_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/callback_list.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/sequence_checker.h"
#include "chrome/browser/media/router/providers/cast/parsed_media_source.h"
#include "chrome/common/media_router/discovery/media_sink_internal.h"
#include "chrome/common/media_router/discovery/media_sink_service_util.h"
#include "chrome/common/media_router/media_source.h"

namespace url {
class Origin;
}

namespace media_router {

class CastMediaSinkService;
class DialMediaSinkService;

// This class uses DialMediaSinkService and CastMediaSinkService to discover
// sinks used by the Cast MediaRouteProvider. It also encapsulates the setup
// necessary to enable dual discovery on Dial/CastMediaSinkService.
// All methods must be called on the UI thread.
class DualMediaSinkService {
 public:
  // Callback / Subscription used for sink discovery (e.g., for MediaRouter to
  // provide sinks to the extension.)
  // Arg 0: Provider name ("dial" or "cast").
  // Arg 1: List of sinks for the provider.
  using OnSinksDiscoveredProviderCallback =
      base::RepeatingCallback<void(const std::string&,
                                   const std::vector<MediaSinkInternal>&)>;
  using OnSinksDiscoveredProviderCallbackList =
      base::CallbackList<void(const std::string&,
                              const std::vector<MediaSinkInternal>&)>;
  using SinkDiscoverySubscription =
      std::unique_ptr<OnSinksDiscoveredProviderCallbackList::Subscription>;

  // Callback / Subscription used for sink queries.
  using SinkQueryCallbackList =
      base::CallbackList<void(const MediaSource::Id&,
                              const std::vector<MediaSinkInternal>&,
                              const std::vector<url::Origin>&)>;
  using SinkQuerySubscription =
      std::unique_ptr<SinkQueryCallbackList::Subscription>;

  // Returns the lazily-created leaky singleton instance.
  static DualMediaSinkService* GetInstance();

  // Returns the current list of sinks, keyed by provider name.
  const base::flat_map<std::string, std::vector<MediaSinkInternal>>&
  current_sinks() {
    return current_sinks_;
  }

  // Adds |callback| to be notified when the list of discovered sinks changes.
  // The caller is responsible for destroying the returned Subscription when it
  // no longer wishes to receive updates. This is used by MediaRouter to provide
  // discovered sinks to the extension.
  SinkDiscoverySubscription AddSinksDiscoveredCallback(
      const OnSinksDiscoveredProviderCallback& callback);

  SinkQuerySubscription AddSinkQueryCallback(const SinkQueryCallback& callback);

  void OnUserGesture();

  // Starts mDNS discovery on |cast_media_sink_service_| if it is not already
  // started.
  void StartMdnsDiscovery();

  void StartObservingMediaSinks(const MediaSource::Id& source_id);
  void StopObservingMediaSinks(const MediaSource::Id& source_id);

 private:
  friend class DualMediaSinkServiceTest;
  FRIEND_TEST_ALL_PREFIXES(DualMediaSinkServiceTest,
                           AddSinksDiscoveredCallback);
  template <bool>
  friend class MediaRouterDesktopTestBase;
  FRIEND_TEST_ALL_PREFIXES(MediaRouterDesktopTest, ProvideSinks);

  friend struct std::default_delete<DualMediaSinkService>;

  DualMediaSinkService();

  // Used by tests.
  DualMediaSinkService(
      std::unique_ptr<CastMediaSinkService> cast_media_sink_service,
      std::unique_ptr<DialMediaSinkService> dial_media_sink_service);

  ~DualMediaSinkService();

  void OnSinksDiscovered(const std::string& provider_name,
                         std::vector<MediaSinkInternal> sinks);
  void OnSinksAvailable(const MediaSource::Id& source_id,
                        const std::vector<MediaSinkInternal>& sinks,
                        const std::vector<url::Origin>& origins);

  std::unique_ptr<CastMediaSinkService> cast_media_sink_service_;
  std::unique_ptr<DialMediaSinkService> dial_media_sink_service_;

  OnSinksDiscoveredProviderCallbackList sinks_discovered_callbacks_;
  base::flat_map<std::string, std::vector<MediaSinkInternal>> current_sinks_;

  // Cast sink queries.
  base::flat_set<MediaSource::Id> registered_sources_;
  SinkQueryCallbackList sink_query_callbacks_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(DualMediaSinkService);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_DUAL_MEDIA_SINK_SERVICE_H_
