// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_CAST_MEDIA_SINK_SERVICE_H_
#define CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_CAST_MEDIA_SINK_SERVICE_H_

#include <map>
#include <memory>
#include <set>

#include "base/gtest_prod_util.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "chrome/browser/media/router/discovery/discovery_network_monitor.h"
#include "chrome/browser/media/router/discovery/mdns/dns_sd_delegate.h"
#include "chrome/browser/media/router/discovery/mdns/dns_sd_registry.h"
#include "chrome/browser/media/router/discovery/media_sink_service_base.h"
#include "components/cast_channel/cast_channel_enum.h"
#include "components/cast_channel/cast_socket.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/ip_endpoint.h"

namespace cast_channel {
class CastSocketService;
}  // namespace cast_channel

namespace content {
class BrowserContext;
}  // namespace content

namespace media_router {

// A service which can be used to start background discovery and resolution of
// Cast devices.
// Public APIs should be invoked on the UI thread.
class CastMediaSinkService
    : public MediaSinkServiceBase,
      public DnsSdRegistry::DnsSdObserver,
      public base::RefCountedThreadSafe<CastMediaSinkService>,
      public DiscoveryNetworkMonitor::Observer {
 public:
  CastMediaSinkService(const OnSinksDiscoveredCallback& callback,
                       content::BrowserContext* browser_context);

  // Used by unit tests.
  CastMediaSinkService(const OnSinksDiscoveredCallback& callback,
                       cast_channel::CastSocketService* cast_socket_service,
                       DiscoveryNetworkMonitor* network_monitor);

  // mDNS service types.
  static const char kCastServiceType[];

  // MediaSinkService implementation
  void Start() override;
  void Stop() override;

 protected:
  // Used to mock out the DnsSdRegistry for testing.
  void SetDnsSdRegistryForTest(DnsSdRegistry* registry);

  ~CastMediaSinkService() override;

 private:
  // Receives incoming messages and errors and provides additional API context.
  class CastSocketObserver : public cast_channel::CastSocket::Observer {
   public:
    explicit CastSocketObserver(CastMediaSinkService* media_sink_service);
    ~CastSocketObserver() override;

    // CastSocket::Observer implementation.
    void OnError(const cast_channel::CastSocket& socket,
                 cast_channel::ChannelError error_state) override;
    void OnMessage(const cast_channel::CastSocket& socket,
                   const cast_channel::CastMessage& message) override;

   private:
    CastMediaSinkService* const media_sink_service_;
    DISALLOW_COPY_AND_ASSIGN(CastSocketObserver);
  };

  friend class base::RefCountedThreadSafe<CastMediaSinkService>;
  friend class CastMediaSinkServiceTest;

  FRIEND_TEST_ALL_PREFIXES(CastMediaSinkServiceTest, TestReStartAfterStop);
  FRIEND_TEST_ALL_PREFIXES(CastMediaSinkServiceTest, TestMultipleStartAndStop);
  FRIEND_TEST_ALL_PREFIXES(CastMediaSinkServiceTest,
                           TestOnChannelOpenedOnIOThread);
  FRIEND_TEST_ALL_PREFIXES(CastMediaSinkServiceTest,
                           TestMultipleOnChannelOpenedOnIOThread);
  FRIEND_TEST_ALL_PREFIXES(CastMediaSinkServiceTest, TestTimer);

  // Attempt to open a cast channel for each service in |current_services_|.
  void StartDeviceResolution(
      const DnsSdRegistry::DnsSdServiceList& services_to_resolve);

  // DnsSdRegistry::DnsSdObserver implementation
  void OnDnsSdEvent(const std::string& service_type,
                    const DnsSdRegistry::DnsSdServiceList& services) override;

  // Opens cast channel on IO thread.
  // |service|: mDNS service description.
  // |ip_endpoint|: cast channel's target IP endpoint.
  void OpenChannelOnIOThread(const DnsSdService& service,
                             const net::IPEndPoint& ip_endpoint);

  // Invoked when opening cast channel on IO thread completes.
  // |service|: mDNS service description.
  // |socket|: raw pointer of newly created cast channel. Does not take
  // ownership of |socket|.
  void OnChannelOpenedOnIOThread(const DnsSdService& service,
                                 cast_channel::CastSocket* socket);

  // Invoked by |OnChannelOpenedOnIOThread| to post task on UI thread.
  // |service|: mDNS service description.
  // |channel_id|: channel id of newly created cast channel.
  // |audio_only|: if cast channel is audio only or not.
  void OnChannelOpenedOnUIThread(const DnsSdService& service,
                                 int channel_id,
                                 bool audio_only);

  // DiscoveryNetworkMonitor::Observer implementation
  void OnNetworksChanged(const std::string& network_id) override;

  void OnCastChannelError(const net::IPEndPoint& ip_endpoint);

  // Raw pointer to DnsSdRegistry instance, which is a global leaky singleton
  // and lives as long as the browser process.
  DnsSdRegistry* dns_sd_registry_ = nullptr;

  // Service list from current round of discovery.
  DnsSdRegistry::DnsSdServiceList current_services_;

  // When reconnecting to a known network, there may be services both from
  // discovery and the cache that need to be resolved.  This could result in
  // duplicate definitions for the same service.  Services in
  // |services_pending_resolution_| came from either the cache or discovery and
  // are waiting for their CastSocket to finish opening.  On success, they will
  // move to |current_services_|.  The cache cannot add a service that is
  // already in |services_pending_resolution_| but discovery can.
  DnsSdRegistry::DnsSdServiceList services_pending_resolution_;

  // Raw pointer of leaky singleton CastSocketService, which manages creating
  // and removing Cast sockets.
  cast_channel::CastSocketService* const cast_socket_service_;

  // Raw pointer to DiscoveryNetworkMonitor, which is a global leaky singleton
  // and manages network change notifications.
  DiscoveryNetworkMonitor* network_monitor_ = nullptr;

  std::unique_ptr<cast_channel::CastSocket::Observer,
                  content::BrowserThread::DeleteOnIOThread>
      observer_;

  std::string current_network_id_ = DiscoveryNetworkMonitor::kNetworkIdUnknown;
  // Cache of known services by network ID.
  std::map<std::string, DnsSdRegistry::DnsSdServiceList> service_cache_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(CastMediaSinkService);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_CAST_MEDIA_SINK_SERVICE_H_
