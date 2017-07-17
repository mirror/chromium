// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_CAST_MEDIA_SINK_SERVICE_IMPL_H_
#define CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_CAST_MEDIA_SINK_SERVICE_IMPL_H_

#include <memory>
#include <set>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "chrome/browser/media/router/discovery/media_sink_service_base.h"
#include "components/cast_channel/cast_channel_enum.h"
#include "components/cast_channel/cast_socket.h"

namespace cast_channel {
class CastSocketService;
}  // namespace cast_channel

namespace media_router {

// A service used to open/close cast channels for Cast devices. This class is
// not thread safe and should be invoked only on the IO thread.
class CastMediaSinkServiceImpl
    : public MediaSinkServiceBase,
      public base::SupportsWeakPtr<CastMediaSinkServiceImpl> {
 public:
  CastMediaSinkServiceImpl(
      const OnSinksDiscoveredCallback& callback,
      cast_channel::CastSocketService* cast_socket_service);
  ~CastMediaSinkServiceImpl() override;

  // Default Cast control port to open Cast Socket from DIAL sink.
  static const int kCastControlPort;

  // MediaSinkService implementation
  void Start() override;
  void Stop() override;

  // Opens cast channels on the IO thread.
  virtual void OpenChannels(std::vector<MediaSinkInternal> cast_sinks);

 private:
  friend class CastMediaSinkServiceImplTest;
  FRIEND_TEST_ALL_PREFIXES(CastMediaSinkServiceImplTest, TestOnChannelOpened);
  FRIEND_TEST_ALL_PREFIXES(CastMediaSinkServiceImplTest,
                           TestMultipleOnChannelOpened);
  FRIEND_TEST_ALL_PREFIXES(CastMediaSinkServiceImplTest, TestTimer);
  FRIEND_TEST_ALL_PREFIXES(CastMediaSinkServiceImplTest,
                           TestMultipleOpenChannels);

  // Receives incoming messages and errors.
  class CastSocketObserver : public cast_channel::CastSocket::Observer {
   public:
    CastSocketObserver();
    ~CastSocketObserver() override;

    // CastSocket::Observer implementation.
    void OnError(const cast_channel::CastSocket& socket,
                 cast_channel::ChannelError error_state) override;
    void OnMessage(const cast_channel::CastSocket& socket,
                   const cast_channel::CastMessage& message) override;

   private:
    DISALLOW_COPY_AND_ASSIGN(CastSocketObserver);
  };

  // Opens cast channel on IO thread.
  // |ip_endpoint|: cast channel's target IP endpoint.
  // |cast_sink|: Cast sink created from mDNS service description or DIAL sink.
  void OpenChannel(const net::IPEndPoint& ip_endpoint,
                   MediaSinkInternal cast_sink);

  // Invoked when opening cast channel on IO thread completes.
  // |cast_sink|: Cast sink created from mDNS service description or DIAL sink.
  // |channel_id|: channel id of newly created cast channel.
  // |channel_error|: error encounted when opending cast channel.
  void OnChannelOpened(MediaSinkInternal cast_sink,
                       int channel_id,
                       cast_channel::ChannelError channel_error);

  // Set of mDNS service ip endpoints from current round of discovery.
  std::set<net::IPEndPoint> current_service_ip_endpoints_;

  // Raw pointer of leaky singleton CastSocketService, which manages creating
  // and removing Cast sockets.
  cast_channel::CastSocketService* const cast_socket_service_;

  std::unique_ptr<cast_channel::CastSocket::Observer> observer_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(CastMediaSinkServiceImpl);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_CAST_MEDIA_SINK_SERVICE_IMPL_H_
