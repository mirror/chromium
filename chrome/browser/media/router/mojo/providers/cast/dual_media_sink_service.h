// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_MOJO_PROVIDERS_CAST_DUAL_MEDIA_SINK_SERVICE_H_
#define CHROME_BROWSER_MEDIA_ROUTER_MOJO_PROVIDERS_CAST_DUAL_MEDIA_SINK_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/callback_list.h"
#include "base/containers/flat_map.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/sequence_checker.h"
#include "chrome/common/media_router/discovery/media_sink_internal.h"

namespace media_router {

class CastMediaSinkService;
class DialMediaSinkService;

class DualMediaSinkService {
 public:
  static DualMediaSinkService* GetInstance();

  using OnSinksDiscoveredProviderCallback =
      base::RepeatingCallback<void(const std::string&,
                                   const std::vector<MediaSinkInternal>&)>;
  using OnSinksDiscoveredProviderCallbackList =
      base::CallbackList<void(const std::string&,
                              const std::vector<MediaSinkInternal>&)>;
  using Subscription =
      std::unique_ptr<OnSinksDiscoveredProviderCallbackList::Subscription>;

  ~DualMediaSinkService();

  Subscription AddSinksDiscoveredCallback(
      const OnSinksDiscoveredProviderCallback& callback);
  const base::flat_map<std::string, std::vector<MediaSinkInternal>>&
  current_sinks() {
    return current_sinks_;
  }

  void OnUserGesture();

 private:
  friend class DualMediaSinkServiceTest;
  FRIEND_TEST_ALL_PREFIXES(DualMediaSinkServiceTest,
                           AddSinksDiscoveredCallback);
  friend class MediaRouterDesktopTest;
  FRIEND_TEST_ALL_PREFIXES(MediaRouterDesktopTest, ProvideSinks);

  DualMediaSinkService();

  // Used by tests.
  DualMediaSinkService(
      std::unique_ptr<CastMediaSinkService> cast_media_sink_service,
      std::unique_ptr<DialMediaSinkService> dial_media_sink_service);

  void OnSinksDiscovered(const std::string& provider_name,
                         std::vector<MediaSinkInternal> sinks);

  std::unique_ptr<CastMediaSinkService> cast_media_sink_service_;
  std::unique_ptr<DialMediaSinkService> dial_media_sink_service_;

  OnSinksDiscoveredProviderCallbackList sinks_discovered_callbacks_;
  base::flat_map<std::string, std::vector<MediaSinkInternal>> current_sinks_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(DualMediaSinkService);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_MOJO_PROVIDERS_CAST_DUAL_MEDIA_SINK_SERVICE_H_
