// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_CAST_MEDIA_SINK_SERVICE_H_
#define CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_CAST_MEDIA_SINK_SERVICE_H_

#include <memory>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/media/router/discovery/dial/dial_media_sink_service_impl.h"
#include "chrome/browser/media/router/discovery/mdns/dns_sd_delegate.h"
#include "chrome/browser/media/router/discovery/mdns/dns_sd_registry.h"
#include "chrome/common/media_router/discovery/media_sink_internal.h"
#include "chrome/common/media_router/discovery/media_sink_service.h"
#include "content/public/browser/browser_thread.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace media_router {

class CastMediaSinkServiceImpl;

// A service which can be used to start background discovery and resolution of
// Cast devices.
// Public APIs should be invoked on the UI thread.
class CastMediaSinkService : public DnsSdRegistry::DnsSdObserver {
 public:
  // mDNS service types.
  static const char kCastServiceType[];

  explicit CastMediaSinkService(content::BrowserContext* browser_context);

  // Used by unit tests.
  explicit CastMediaSinkService(
      std::unique_ptr<CastMediaSinkServiceImpl> cast_media_sink_service_impl);

  ~CastMediaSinkService() override;

  OnDialSinkAddedCallback GetOnDialSinkAddedCallback();

  void Start(const OnSinksDiscoveredCallback& sinks_discovered_cb);
  void Stop();
  void ForceSinkDiscoveryCallback();
  void OnUserGesture();

  void SetDnsSdRegistryForTest(DnsSdRegistry* registry);

 private:
  friend class CastMediaSinkServiceTest;

  FRIEND_TEST_ALL_PREFIXES(CastMediaSinkServiceTest, TestRestartAfterStop);
  FRIEND_TEST_ALL_PREFIXES(CastMediaSinkServiceTest, TestMultipleStartAndStop);
  FRIEND_TEST_ALL_PREFIXES(CastMediaSinkServiceTest, TestOnDnsSdEvent);
  FRIEND_TEST_ALL_PREFIXES(CastMediaSinkServiceTest, TestTimer);

  void OnDialSinkAdded(const MediaSinkInternal& sink);

  // DnsSdRegistry::DnsSdObserver implementation
  void OnDnsSdEvent(const std::string& service_type,
                    const DnsSdRegistry::DnsSdServiceList& services) override;

  // Raw pointer to DnsSdRegistry instance, which is a global leaky singleton
  // and lives as long as the browser process.
  DnsSdRegistry* dns_sd_registry_ = nullptr;

  // Created on the UI thread, used and destroyed on its SequencedTaskRunner.
  std::unique_ptr<CastMediaSinkServiceImpl> cast_media_sink_service_impl_;

  // List of cast sinks found in current round of mDNS discovery.
  std::vector<MediaSinkInternal> cast_sinks_;

  content::BrowserContext* browser_context_ = nullptr;

  base::WeakPtrFactory<CastMediaSinkService> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CastMediaSinkService);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_CAST_MEDIA_SINK_SERVICE_H_
