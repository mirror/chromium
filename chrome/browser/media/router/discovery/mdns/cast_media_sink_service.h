// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_CAST_MEDIA_SINK_SERVICE_H_
#define CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_CAST_MEDIA_SINK_SERVICE_H_

#include <memory>

#include "base/gtest_prod_util.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/media/router/discovery/mdns/dns_sd_delegate.h"
#include "chrome/browser/media/router/discovery/mdns/dns_sd_registry.h"
#include "chrome/common/media_router/discovery/media_sink_service.h"

namespace cast_channel {
class CastSocketService;
}  // namespace cast_channel

namespace content {
class BrowserContext;
}  // namespace content

namespace media_router {

class CastMediaSinkServiceImpl;

// A service which can be used to start background discovery and resolution of
// Cast devices.
// Public APIs should be invoked on the UI thread.
class CastMediaSinkService
    : public MediaSinkService,
      public DnsSdRegistry::DnsSdObserver,
      public base::RefCountedThreadSafe<CastMediaSinkService> {
 public:
  CastMediaSinkService(const OnSinksDiscoveredCallback& callback,
                       content::BrowserContext* browser_context);

  // mDNS service types.
  static const char kCastServiceType[];

  // MediaSinkService implementation
  void Start() override;
  void Stop() override;

  void SetDnsSdRegistryForTest(DnsSdRegistry* registry);
  void SetCastMediaSinkServiceImplForTest(
      std::unique_ptr<CastMediaSinkServiceImpl> cast_media_sink_service_impl);

 protected:
  ~CastMediaSinkService() override;

 private:

  friend class base::RefCountedThreadSafe<CastMediaSinkService>;
  friend class CastMediaSinkServiceTest;

  FRIEND_TEST_ALL_PREFIXES(CastMediaSinkServiceTest, TestRestartAfterStop);
  FRIEND_TEST_ALL_PREFIXES(CastMediaSinkServiceTest, TestMultipleStartAndStop);
  FRIEND_TEST_ALL_PREFIXES(CastMediaSinkServiceTest, TestOnDnsSdEvent);
  FRIEND_TEST_ALL_PREFIXES(CastMediaSinkServiceTest, TestTimer);

  // Starts DIAL discovery.
  void StartOnIOThread();

  // Stops DIAL discovery.
  void StopOnIOThread();

  // Opens cast channels on the IO thread.
  void OpenChannelsOnIOThread(std::vector<MediaSinkInternal> cast_sinks);

  void OnSinksDiscoveredOnIOThread(std::vector<MediaSinkInternal> sinks);

  // DnsSdRegistry::DnsSdObserver implementation
  void OnDnsSdEvent(const std::string& service_type,
                    const DnsSdRegistry::DnsSdServiceList& services) override;

  // Raw pointer to DnsSdRegistry instance, which is a global leaky singleton
  // and lives as long as the browser process.
  DnsSdRegistry* dns_sd_registry_ = nullptr;

  // Created and destroyed on the IO thread.
  std::unique_ptr<CastMediaSinkServiceImpl> cast_media_sink_service_impl_;

  // Raw pointer of leaky singleton CastSocketService, which manages creating
  // and removing Cast sockets.
  cast_channel::CastSocketService* cast_socket_service_;

  DISALLOW_COPY_AND_ASSIGN(CastMediaSinkService);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_CAST_MEDIA_SINK_SERVICE_H_
