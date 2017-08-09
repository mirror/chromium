// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/mdns/cast_media_sink_service_impl.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/test/mock_callback.h"
#include "base/timer/mock_timer.h"
#include "chrome/browser/media/router/test_helper.h"
#include "components/cast_channel/cast_socket.h"
#include "components/cast_channel/cast_socket_service.h"
#include "components/cast_channel/cast_test_util.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::Invoke;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::_;

namespace {

net::IPEndPoint CreateIPEndPoint(int num) {
  net::IPAddress ip_address;
  CHECK(ip_address.AssignFromIPLiteral(
      base::StringPrintf("192.168.0.10%d", num)));
  return net::IPEndPoint(ip_address, 8009 + num);
}

media_router::MediaSinkInternal CreateCastSink(int num) {
  std::string friendly_name = base::StringPrintf("friendly name %d", num);
  std::string unique_id = base::StringPrintf("id %d", num);
  net::IPEndPoint ip_endpoint = CreateIPEndPoint(num);

  media_router::MediaSink sink(unique_id, friendly_name,
                               media_router::SinkIconType::CAST);
  media_router::CastSinkExtraData extra_data;
  extra_data.ip_address = ip_endpoint.address();
  extra_data.port = ip_endpoint.port();
  extra_data.model_name = base::StringPrintf("model name %d", num);
  extra_data.cast_channel_id = num;
  extra_data.capabilities = cast_channel::CastDeviceCapability::AUDIO_OUT |
                            cast_channel::CastDeviceCapability::VIDEO_OUT;
  return media_router::MediaSinkInternal(sink, extra_data);
}

}  // namespace

namespace media_router {

class CastMediaSinkServiceImplTest : public ::testing::Test {
 public:
  CastMediaSinkServiceImplTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        mock_cast_socket_service_(new cast_channel::MockCastSocketService()),
        media_sink_service_impl_(mock_sink_discovered_cb_.Get(),
                                 mock_cast_socket_service_.get(),
                                 discovery_network_monitor_.get()) {}

  void SetUp() override {
    fake_network_info_.clear();
    auto mock_timer = base::MakeUnique<base::MockTimer>(
        true /*retain_user_task*/, false /*is_repeating*/);
    mock_timer_ = mock_timer.get();
    media_sink_service_impl_.SetTimerForTest(std::move(mock_timer));
  }

 protected:
  static std::vector<DiscoveryNetworkInfo> FakeGetNetworkInfo() {
    return fake_network_info_;
  }

  static std::vector<DiscoveryNetworkInfo> fake_network_info_;

  std::vector<DiscoveryNetworkInfo> fake_ethernet_info_{
      {{std::string("enp0s2"), std::string("ethernet1")}}};
  std::vector<DiscoveryNetworkInfo> fake_wifi_info_{
      {DiscoveryNetworkInfo{std::string("wlp3s0"), std::string("wifi1")},
       DiscoveryNetworkInfo{std::string("wlp3s1"), std::string("wifi2")}}};
  std::vector<DiscoveryNetworkInfo> fake_unknown_info_{
      {{std::string("enp0s2"), std::string()}}};

  std::unique_ptr<net::NetworkChangeNotifier> network_change_notifier_ =
      base::WrapUnique(net::NetworkChangeNotifier::CreateMock());

  const content::TestBrowserThreadBundle thread_bundle_;

  std::unique_ptr<DiscoveryNetworkMonitor> discovery_network_monitor_ =
      DiscoveryNetworkMonitor::CreateInstanceForTest(&FakeGetNetworkInfo);

  base::MockCallback<MediaSinkService::OnSinksDiscoveredCallback>
      mock_sink_discovered_cb_;
  std::unique_ptr<cast_channel::MockCastSocketService>
      mock_cast_socket_service_;
  CastMediaSinkServiceImpl media_sink_service_impl_;
  base::MockTimer* mock_timer_;

  DISALLOW_COPY_AND_ASSIGN(CastMediaSinkServiceImplTest);
};

// static
std::vector<DiscoveryNetworkInfo>
    CastMediaSinkServiceImplTest::fake_network_info_;

TEST_F(CastMediaSinkServiceImplTest, TestOnChannelOpened) {
  auto cast_sink = CreateCastSink(1);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);
  cast_channel::MockCastSocket socket;
  socket.set_id(1);

  media_sink_service_impl_.current_service_ip_endpoints_.insert(ip_endpoint1);
  media_sink_service_impl_.OnChannelOpened(cast_sink, &socket);

  // Verify sink content
  EXPECT_EQ(media_sink_service_impl_.current_sinks_,
            std::set<MediaSinkInternal>({cast_sink}));
}

TEST_F(CastMediaSinkServiceImplTest, TestMultipleOnChannelOpened) {
  auto cast_sink1 = CreateCastSink(1);
  auto cast_sink2 = CreateCastSink(2);
  auto cast_sink3 = CreateCastSink(3);
  auto ip_endpoint1 = CreateIPEndPoint(1);
  auto ip_endpoint2 = CreateIPEndPoint(2);
  auto ip_endpoint3 = CreateIPEndPoint(3);

  cast_channel::MockCastSocket socket2;
  socket2.set_id(2);
  cast_channel::MockCastSocket socket3;
  socket3.set_id(3);

  // Current round of Dns discovery finds service1 and service 2.
  media_sink_service_impl_.current_service_ip_endpoints_.insert(ip_endpoint1);
  media_sink_service_impl_.current_service_ip_endpoints_.insert(ip_endpoint2);
  // Fail to open channel 1.
  media_sink_service_impl_.OnChannelOpened(cast_sink2, &socket2);
  media_sink_service_impl_.OnChannelOpened(cast_sink3, &socket3);

  EXPECT_EQ(media_sink_service_impl_.current_sinks_,
            std::set<MediaSinkInternal>({cast_sink2, cast_sink3}));
}

TEST_F(CastMediaSinkServiceImplTest, TestTimer) {
  auto cast_sink1 = CreateCastSink(1);
  auto cast_sink2 = CreateCastSink(2);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);
  net::IPEndPoint ip_endpoint2 = CreateIPEndPoint(2);

  EXPECT_FALSE(mock_timer_->IsRunning());
  media_sink_service_impl_.Start();
  EXPECT_TRUE(mock_timer_->IsRunning());

  // Channel 2 is opened.
  cast_channel::MockCastSocket socket2;
  socket2.set_id(2);

  media_sink_service_impl_.current_service_ip_endpoints_.insert(ip_endpoint2);
  media_sink_service_impl_.OnChannelOpened(cast_sink2, &socket2);

  std::vector<MediaSinkInternal> sinks;
  EXPECT_CALL(mock_sink_discovered_cb_, Run(_)).WillOnce(SaveArg<0>(&sinks));

  // Fire timer.
  mock_timer_->Fire();
  EXPECT_EQ(sinks, std::vector<MediaSinkInternal>({cast_sink2}));

  EXPECT_FALSE(mock_timer_->IsRunning());
  // Channel 1 is opened and timer is restarted.
  cast_channel::MockCastSocket socket1;
  socket1.set_id(1);

  media_sink_service_impl_.current_service_ip_endpoints_.insert(ip_endpoint1);
  media_sink_service_impl_.OnChannelOpened(cast_sink1, &socket1);
  EXPECT_TRUE(mock_timer_->IsRunning());
}

TEST_F(CastMediaSinkServiceImplTest, TestMultipleOpenChannels) {
  auto cast_sink1 = CreateCastSink(1);
  auto cast_sink2 = CreateCastSink(2);
  auto cast_sink3 = CreateCastSink(3);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);
  net::IPEndPoint ip_endpoint2 = CreateIPEndPoint(2);
  net::IPEndPoint ip_endpoint3 = CreateIPEndPoint(3);

  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _));
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _));

  // 1st round finds service 1 & 2.
  std::vector<MediaSinkInternal> sinks1{cast_sink1, cast_sink2};
  media_sink_service_impl_.OpenChannels(sinks1);

  // Channel 2 opened.
  cast_channel::MockCastSocket socket2;
  socket2.set_id(2);
  media_sink_service_impl_.OnChannelOpened(cast_sink2, &socket2);

  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _));
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint3, _, _, _));

  // 2nd round finds service 2 & 3.
  std::vector<MediaSinkInternal> sinks2{cast_sink2, cast_sink3};
  media_sink_service_impl_.OpenChannels(sinks2);

  // Channel 1 and 3 opened.
  cast_channel::MockCastSocket socket1;
  cast_channel::MockCastSocket socket3;
  socket1.set_id(1);
  socket3.set_id(3);
  media_sink_service_impl_.OnChannelOpened(cast_sink1, &socket1);
  media_sink_service_impl_.OnChannelOpened(cast_sink3, &socket3);

  EXPECT_EQ(media_sink_service_impl_.current_sinks_,
            std::set<MediaSinkInternal>({cast_sink1, cast_sink2, cast_sink3}));
}

TEST_F(CastMediaSinkServiceImplTest, TestOnChannelError) {
  auto cast_sink = CreateCastSink(1);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);
  cast_channel::MockCastSocket socket;
  socket.set_id(1);

  media_sink_service_impl_.current_service_ip_endpoints_.insert(ip_endpoint1);
  media_sink_service_impl_.OnChannelOpened(cast_sink, &socket);

  EXPECT_EQ(1u, media_sink_service_impl_.current_sinks_.size());

  socket.SetIPEndpoint(ip_endpoint1);
  static_cast<cast_channel::CastSocket::Observer&>(media_sink_service_impl_)
      .OnError(socket, cast_channel::ChannelError::CHANNEL_NOT_OPEN);
  EXPECT_TRUE(media_sink_service_impl_.current_sinks_.empty());
}

TEST_F(CastMediaSinkServiceImplTest, CacheSinksForKnownNetwork) {
  fake_network_info_ = fake_ethernet_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  content::RunAllBlockingPoolTasksUntilIdle();

  MediaSinkInternal sink1 = CreateCastSink(1);
  MediaSinkInternal sink2 = CreateCastSink(2);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);
  net::IPEndPoint ip_endpoint2 = CreateIPEndPoint(2);
  std::vector<MediaSinkInternal> sink_list1{sink1, sink2};

  // Resolution will succeed for both sinks.
  cast_channel::MockCastSocket socket1;
  cast_channel::MockCastSocket socket2;
  socket1.SetIPEndpoint(ip_endpoint1);
  socket1.set_id(1);
  socket2.SetIPEndpoint(ip_endpoint2);
  socket2.set_id(2);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .WillOnce(DoAll(Invoke([&socket1](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket1);
                      }),
                      Return(1)));
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _))
      .WillOnce(DoAll(Invoke([&socket2](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket2);
                      }),
                      Return(2)));
  media_sink_service_impl_.OpenChannels(sink_list1);

  // Connect to a new network with different sinks.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  fake_network_info_ = fake_wifi_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  content::RunAllBlockingPoolTasksUntilIdle();
  static_cast<cast_channel::CastSocket::Observer&>(media_sink_service_impl_)
      .OnError(socket1, cast_channel::ChannelError::CAST_SOCKET_ERROR);
  static_cast<cast_channel::CastSocket::Observer&>(media_sink_service_impl_)
      .OnError(socket2, cast_channel::ChannelError::CAST_SOCKET_ERROR);

  MediaSinkInternal sink3 = CreateCastSink(3);
  net::IPEndPoint ip_endpoint3 = CreateIPEndPoint(3);
  std::vector<MediaSinkInternal> sink_list2{sink3};

  cast_channel::MockCastSocket socket3;
  socket3.SetIPEndpoint(ip_endpoint3);
  socket3.set_id(3);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint3, _, _, _))
      .WillOnce(DoAll(Invoke([&socket3](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket3);
                      }),
                      Return(3)));
  media_sink_service_impl_.OpenChannels(sink_list2);

  // Reconnecting to the previous ethernet network should restore the same sinks
  // from the cache and attempt to resolve them.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _));
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _));
  fake_network_info_ = fake_ethernet_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  content::RunAllBlockingPoolTasksUntilIdle();
}

TEST_F(CastMediaSinkServiceImplTest, CacheContainsOnlyResolvedSinks) {
  fake_network_info_ = fake_ethernet_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  content::RunAllBlockingPoolTasksUntilIdle();

  MediaSinkInternal sink1 = CreateCastSink(1);
  MediaSinkInternal sink2 = CreateCastSink(2);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);
  net::IPEndPoint ip_endpoint2 = CreateIPEndPoint(2);
  std::vector<MediaSinkInternal> sink_list1{sink1, sink2};

  // Resolution will fail for |sink2|.
  cast_channel::MockCastSocket socket1;
  cast_channel::MockCastSocket socket2;
  socket1.SetIPEndpoint(ip_endpoint1);
  socket1.set_id(1);
  socket2.SetErrorState(cast_channel::ChannelError::CONNECT_ERROR);
  socket2.set_id(2);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .WillOnce(DoAll(Invoke([&socket1](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket1);
                      }),
                      Return(1)));
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _))
      .WillOnce(DoAll(Invoke([&socket2](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket2);
                      }),
                      Return(2)));
  media_sink_service_impl_.OpenChannels(sink_list1);

  // Connect to a new network with different sinks.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  fake_network_info_ = fake_wifi_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  content::RunAllBlockingPoolTasksUntilIdle();
  static_cast<cast_channel::CastSocket::Observer&>(media_sink_service_impl_)
      .OnError(socket1, cast_channel::ChannelError::CAST_SOCKET_ERROR);

  MediaSinkInternal sink3 = CreateCastSink(3);
  net::IPEndPoint ip_endpoint3 = CreateIPEndPoint(3);
  std::vector<MediaSinkInternal> sink_list2{sink3};

  cast_channel::MockCastSocket socket3;
  socket3.SetIPEndpoint(ip_endpoint3);
  socket3.set_id(3);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint3, _, _, _))
      .WillOnce(DoAll(Invoke([&socket3](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket3);
                      }),
                      Return(3)));
  media_sink_service_impl_.OpenChannels(sink_list2);

  // Reconnecting to the previous ethernet network should restore only |sink1|,
  // since |sink2| failed to resolve.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _));
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _))
      .Times(0);
  fake_network_info_ = fake_ethernet_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  content::RunAllBlockingPoolTasksUntilIdle();
}

TEST_F(CastMediaSinkServiceImplTest, CacheUpdatedOnChannelError) {
  fake_network_info_ = fake_ethernet_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  content::RunAllBlockingPoolTasksUntilIdle();

  MediaSinkInternal sink1 = CreateCastSink(1);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);
  std::vector<MediaSinkInternal> sink_list1{sink1};

  // Resolve |sink1| but then raise a channel error.  This should remove it from
  // the cached sinks for the ethernet network.
  cast_channel::MockCastSocket socket1;
  socket1.SetIPEndpoint(ip_endpoint1);
  socket1.set_id(1);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .WillOnce(DoAll(Invoke([&socket1](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket1);
                      }),
                      Return(1)));
  media_sink_service_impl_.OpenChannels(sink_list1);
  static_cast<cast_channel::CastSocket::Observer&>(media_sink_service_impl_)
      .OnError(socket1, cast_channel::ChannelError::CAST_SOCKET_ERROR);

  // Connect to a new network with different sinks.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  fake_network_info_ = fake_wifi_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  content::RunAllBlockingPoolTasksUntilIdle();

  MediaSinkInternal sink2 = CreateCastSink(2);
  net::IPEndPoint ip_endpoint2 = CreateIPEndPoint(2);
  std::vector<MediaSinkInternal> sink_list2{sink2};

  cast_channel::MockCastSocket socket2;
  socket2.SetIPEndpoint(ip_endpoint2);
  socket2.set_id(2);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _))
      .WillOnce(DoAll(Invoke([&socket2](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket2);
                      }),
                      Return(2)));
  media_sink_service_impl_.OpenChannels(sink_list2);

  // Reconnecting to the previous ethernet network should not restore any sinks
  // since the only sink to resolve successfully, |sink1|, later had a channel
  // error.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  EXPECT_CALL(*mock_cast_socket_service_, OpenSocketInternal(_, _, _, _))
      .Times(0);
  fake_network_info_ = fake_ethernet_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  content::RunAllBlockingPoolTasksUntilIdle();
}

TEST_F(CastMediaSinkServiceImplTest, UnknownNetworkNoCache) {
  // Without any network notification here, network ID will remain __unknown__
  // and the cache shouldn't save any of these sinks.
  MediaSinkInternal sink1 = CreateCastSink(1);
  MediaSinkInternal sink2 = CreateCastSink(2);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);
  net::IPEndPoint ip_endpoint2 = CreateIPEndPoint(2);
  std::vector<MediaSinkInternal> sink_list1{sink1, sink2};

  // Resolution will succeed for both sinks.
  cast_channel::MockCastSocket socket1;
  cast_channel::MockCastSocket socket2;
  socket1.SetIPEndpoint(ip_endpoint1);
  socket1.set_id(1);
  socket2.SetIPEndpoint(ip_endpoint2);
  socket2.set_id(2);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .WillOnce(DoAll(Invoke([&socket1](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket1);
                      }),
                      Return(1)));
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _))
      .WillOnce(DoAll(Invoke([&socket2](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket2);
                      }),
                      Return(2)));
  media_sink_service_impl_.OpenChannels(sink_list1);

  // Connect to a new network with different sinks.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  fake_network_info_ = fake_wifi_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  content::RunAllBlockingPoolTasksUntilIdle();
  static_cast<cast_channel::CastSocket::Observer&>(media_sink_service_impl_)
      .OnError(socket1, cast_channel::ChannelError::CAST_SOCKET_ERROR);
  static_cast<cast_channel::CastSocket::Observer&>(media_sink_service_impl_)
      .OnError(socket2, cast_channel::ChannelError::CAST_SOCKET_ERROR);

  MediaSinkInternal sink3 = CreateCastSink(3);
  net::IPEndPoint ip_endpoint3 = CreateIPEndPoint(3);
  std::vector<MediaSinkInternal> sink_list2{sink3};

  cast_channel::MockCastSocket socket3;
  socket3.SetIPEndpoint(ip_endpoint3);
  socket3.set_id(3);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint3, _, _, _))
      .WillOnce(DoAll(Invoke([&socket3](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket3);
                      }),
                      Return(3)));
  media_sink_service_impl_.OpenChannels(sink_list2);

  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  // Connecting to a network whose ID resolves to __unknown__ shouldn't pull any
  // cache items from another unknown network.
  EXPECT_CALL(*mock_cast_socket_service_, OpenSocketInternal(_, _, _, _))
      .Times(0);
  fake_network_info_ = fake_unknown_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  content::RunAllBlockingPoolTasksUntilIdle();
}

TEST_F(CastMediaSinkServiceImplTest, DisconnectedNoCache) {
  // Discovery can report sinks while we think we're disconnected, but nothing
  // should be cached.
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  MediaSinkInternal sink1 = CreateCastSink(1);
  MediaSinkInternal sink2 = CreateCastSink(2);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);
  net::IPEndPoint ip_endpoint2 = CreateIPEndPoint(2);
  std::vector<MediaSinkInternal> sink_list1{sink1, sink2};

  // Resolution will succeed for both sinks.
  cast_channel::MockCastSocket socket1;
  cast_channel::MockCastSocket socket2;
  socket1.SetIPEndpoint(ip_endpoint1);
  socket1.set_id(1);
  socket2.SetIPEndpoint(ip_endpoint2);
  socket2.set_id(2);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .WillOnce(DoAll(Invoke([&socket1](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket1);
                      }),
                      Return(1)));
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _))
      .WillOnce(DoAll(Invoke([&socket2](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket2);
                      }),
                      Return(2)));
  media_sink_service_impl_.OpenChannels(sink_list1);

  // Connect to a new known network with different sinks.
  fake_network_info_ = fake_wifi_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  content::RunAllBlockingPoolTasksUntilIdle();
  static_cast<cast_channel::CastSocket::Observer&>(media_sink_service_impl_)
      .OnError(socket1, cast_channel::ChannelError::CAST_SOCKET_ERROR);
  static_cast<cast_channel::CastSocket::Observer&>(media_sink_service_impl_)
      .OnError(socket2, cast_channel::ChannelError::CAST_SOCKET_ERROR);

  MediaSinkInternal sink3 = CreateCastSink(3);
  net::IPEndPoint ip_endpoint3 = CreateIPEndPoint(3);
  std::vector<MediaSinkInternal> sink_list2{sink3};

  cast_channel::MockCastSocket socket3;
  socket3.SetIPEndpoint(ip_endpoint3);
  socket3.set_id(3);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint3, _, _, _))
      .WillOnce(DoAll(Invoke([&socket3](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket3);
                      }),
                      Return(3)));
  media_sink_service_impl_.OpenChannels(sink_list2);

  // Connecting to a network whose ID resolves to __unknown__ shouldn't pull any
  // cache items from another unknown network.
  EXPECT_CALL(*mock_cast_socket_service_, OpenSocketInternal(_, _, _, _))
      .Times(0);
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();
}

TEST_F(CastMediaSinkServiceImplTest, CacheUpdatedForKnownNetwork) {
  fake_network_info_ = fake_ethernet_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  content::RunAllBlockingPoolTasksUntilIdle();

  MediaSinkInternal sink1 = CreateCastSink(1);
  MediaSinkInternal sink2 = CreateCastSink(2);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);
  net::IPEndPoint ip_endpoint2 = CreateIPEndPoint(2);
  std::vector<MediaSinkInternal> sink_list1{sink1, sink2};

  // Resolution will succeed for both sinks.
  cast_channel::MockCastSocket socket1;
  cast_channel::MockCastSocket socket2;
  socket1.SetIPEndpoint(ip_endpoint1);
  socket1.set_id(1);
  socket2.SetIPEndpoint(ip_endpoint2);
  socket2.set_id(2);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .WillOnce(DoAll(Invoke([&socket1](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket1);
                      }),
                      Return(1)));
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _))
      .WillOnce(DoAll(Invoke([&socket2](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket2);
                      }),
                      Return(2)));
  media_sink_service_impl_.OpenChannels(sink_list1);

  // Connect to a new network with different sinks.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  fake_network_info_ = fake_wifi_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  content::RunAllBlockingPoolTasksUntilIdle();
  static_cast<cast_channel::CastSocket::Observer&>(media_sink_service_impl_)
      .OnError(socket1, cast_channel::ChannelError::CAST_SOCKET_ERROR);
  static_cast<cast_channel::CastSocket::Observer&>(media_sink_service_impl_)
      .OnError(socket2, cast_channel::ChannelError::CAST_SOCKET_ERROR);

  MediaSinkInternal sink3 = CreateCastSink(3);
  net::IPEndPoint ip_endpoint3 = CreateIPEndPoint(3);
  std::vector<MediaSinkInternal> sink_list2{sink3};

  cast_channel::MockCastSocket socket3;
  socket3.SetIPEndpoint(ip_endpoint3);
  socket3.set_id(3);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint3, _, _, _))
      .WillOnce(DoAll(Invoke([&socket3](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket3);
                      }),
                      Return(3)));
  media_sink_service_impl_.OpenChannels(sink_list2);

  // Reconnecting to the previous ethernet network should restore the same sinks
  // from the cache and attempt to resolve them.  |sink3| is also lost.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  static_cast<cast_channel::CastSocket::Observer&>(media_sink_service_impl_)
      .OnError(socket3, cast_channel::ChannelError::CAST_SOCKET_ERROR);

  // Resolution will fail for cached sinks.
  socket1.SetErrorState(cast_channel::ChannelError::CONNECT_ERROR);
  socket2.SetErrorState(cast_channel::ChannelError::CONNECT_ERROR);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .WillOnce(DoAll(Invoke([&socket1](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket1);
                      }),
                      Return(1)));
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _))
      .WillOnce(DoAll(Invoke([&socket2](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket2);
                      }),
                      Return(2)));
  fake_network_info_ = fake_ethernet_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  content::RunAllBlockingPoolTasksUntilIdle();

  // A new sink is found on the ethernet network.
  MediaSinkInternal sink4 = CreateCastSink(4);
  net::IPEndPoint ip_endpoint4 = CreateIPEndPoint(4);
  std::vector<MediaSinkInternal> sink_list3{sink4};

  cast_channel::MockCastSocket socket4;
  socket4.SetIPEndpoint(ip_endpoint4);
  socket4.set_id(4);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint4, _, _, _))
      .WillOnce(DoAll(Invoke([&socket4](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket4);
                      }),
                      Return(4)));
  media_sink_service_impl_.OpenChannels(sink_list3);

  // Disconnect from the network and lose sinks.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();
  static_cast<cast_channel::CastSocket::Observer&>(media_sink_service_impl_)
      .OnError(socket4, cast_channel::ChannelError::CAST_SOCKET_ERROR);

  // Reconnect and expect only |sink4| to be cached.
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint4, _, _, _));
  fake_network_info_ = fake_ethernet_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  content::RunAllBlockingPoolTasksUntilIdle();
}

TEST_F(CastMediaSinkServiceImplTest, CurrentEndpointsUpdated) {
  MediaSinkInternal sink1 = CreateCastSink(1);
  MediaSinkInternal sink2 = CreateCastSink(2);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);
  net::IPEndPoint ip_endpoint2 = CreateIPEndPoint(2);
  std::vector<MediaSinkInternal> sink_list1{sink1, sink2};

  // Resolution will succeed for both sinks.
  cast_channel::MockCastSocket socket1;
  cast_channel::MockCastSocket socket2;
  socket1.SetIPEndpoint(ip_endpoint1);
  socket1.set_id(1);
  socket2.SetIPEndpoint(ip_endpoint2);
  socket2.set_id(2);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .WillOnce(DoAll(Invoke([&socket1](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket1);
                      }),
                      Return(1)));
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _))
      .WillOnce(DoAll(Invoke([&socket2](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket2);
                      }),
                      Return(2)));
  media_sink_service_impl_.OpenChannels(sink_list1);

  // |sink1| disconnects.
  static_cast<cast_channel::CastSocket::Observer&>(media_sink_service_impl_)
      .OnError(socket1, cast_channel::ChannelError::CAST_SOCKET_ERROR);

  // |sink3| is discovered.
  MediaSinkInternal sink3 = CreateCastSink(3);
  net::IPEndPoint ip_endpoint3 = CreateIPEndPoint(3);
  std::vector<MediaSinkInternal> sink_list2{sink3};

  cast_channel::MockCastSocket socket3;
  socket3.SetIPEndpoint(ip_endpoint3);
  socket3.set_id(3);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint3, _, _, _))
      .WillOnce(DoAll(Invoke([&socket3](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket3);
                      }),
                      Return(3)));
  media_sink_service_impl_.OpenChannels(sink_list2);

  // |sink4| is also discovered later.
  MediaSinkInternal sink4 = CreateCastSink(4);
  net::IPEndPoint ip_endpoint4 = CreateIPEndPoint(4);
  std::vector<MediaSinkInternal> sink_list3{sink4};

  cast_channel::MockCastSocket socket4;
  socket4.SetIPEndpoint(ip_endpoint4);
  socket4.set_id(4);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint4, _, _, _))
      .WillOnce(DoAll(Invoke([&socket4](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        std::move(open_cb).Run(&socket4);
                      }),
                      Return(4)));
  media_sink_service_impl_.OpenChannels(sink_list3);

  // |sink3| disconnects.
  static_cast<cast_channel::CastSocket::Observer&>(media_sink_service_impl_)
      .OnError(socket3, cast_channel::ChannelError::CAST_SOCKET_ERROR);

  // |current_service_ip_endpoints_| should contain |ip_endpoint2| and
  // |ip_endpoint4|.
  auto& current_service_ip_endpoints =
      media_sink_service_impl_.current_service_ip_endpoints_;
  EXPECT_EQ(2u, current_service_ip_endpoints.size());
  EXPECT_EQ(current_service_ip_endpoints,
            std::set<net::IPEndPoint>({ip_endpoint2, ip_endpoint4}));
}

}  // namespace media_router
