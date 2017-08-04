// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/mdns/cast_media_sink_service.h"
#include "base/strings/stringprintf.h"
#include "base/test/mock_callback.h"
#include "base/timer/mock_timer.h"
#include "chrome/browser/media/router/discovery/mdns/mock_dns_sd_registry.h"
#include "chrome/browser/media/router/test_helper.h"
#include "chrome/test/base/testing_profile.h"
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

media_router::DnsSdService CreateDnsService(int num) {
  net::IPEndPoint ip_endpoint = CreateIPEndPoint(num);
  media_router::DnsSdService service;
  service.service_name =
      base::StringPrintf("_myDevice%d.", num) +
      std::string(media_router::CastMediaSinkService::kCastServiceType);
  service.ip_address = ip_endpoint.address().ToString();
  service.service_host_port = net::HostPortPair::FromIPEndPoint(ip_endpoint);
  service.service_data.push_back(base::StringPrintf("id=service %d", num));
  service.service_data.push_back(
      base::StringPrintf("fn=friendly name %d", num));
  service.service_data.push_back(base::StringPrintf("md=model name %d", num));

  return service;
}

void VerifyMediaSinkInternal(const media_router::MediaSinkInternal& cast_sink,
                             const media_router::DnsSdService& service,
                             int channel_id,
                             bool audio_only) {
  std::string id = base::StringPrintf("service %d", channel_id);
  std::string name = base::StringPrintf("friendly name %d", channel_id);
  std::string model_name = base::StringPrintf("model name %d", channel_id);
  EXPECT_EQ(id, cast_sink.sink().id());
  EXPECT_EQ(name, cast_sink.sink().name());
  EXPECT_EQ(model_name, cast_sink.cast_data().model_name);
  EXPECT_EQ(service.ip_address, cast_sink.cast_data().ip_address.ToString());

  int capabilities = cast_channel::CastDeviceCapability::AUDIO_OUT;
  if (!audio_only)
    capabilities |= cast_channel::CastDeviceCapability::VIDEO_OUT;
  EXPECT_EQ(capabilities, cast_sink.cast_data().capabilities);
  EXPECT_EQ(channel_id, cast_sink.cast_data().cast_channel_id);
}

}  // namespace

namespace media_router {

class CastMediaSinkServiceTest : public ::testing::Test {
 public:
  CastMediaSinkServiceTest()
      : mock_cast_socket_service_(new cast_channel::MockCastSocketService()),
        media_sink_service_(
            new CastMediaSinkService(mock_sink_discovered_cb_.Get(),
                                     mock_cast_socket_service_.get(),
                                     discovery_network_monitor_.get())),
        test_dns_sd_registry_(media_sink_service_.get()) {}

  void SetUp() override {
    fake_network_info_.clear();
    auto mock_timer = base::MakeUnique<base::MockTimer>(
        true /*retain_user_task*/, false /*is_repeating*/);
    mock_timer_ = mock_timer.get();
    media_sink_service_->SetTimerForTest(std::move(mock_timer));
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

  TestingProfile profile_;

  base::MockCallback<MediaSinkService::OnSinksDiscoveredCallback>
      mock_sink_discovered_cb_;
  std::unique_ptr<cast_channel::MockCastSocketService>
      mock_cast_socket_service_;
  scoped_refptr<CastMediaSinkService> media_sink_service_;
  MockDnsSdRegistry test_dns_sd_registry_;
  base::MockTimer* mock_timer_;

  DISALLOW_COPY_AND_ASSIGN(CastMediaSinkServiceTest);
};

// static
std::vector<DiscoveryNetworkInfo> CastMediaSinkServiceTest::fake_network_info_;

TEST_F(CastMediaSinkServiceTest, TestReStartAfterStop) {
  EXPECT_CALL(test_dns_sd_registry_, AddObserver(media_sink_service_.get()))
      .Times(2);
  EXPECT_CALL(test_dns_sd_registry_, RegisterDnsSdListener(_)).Times(2);
  EXPECT_FALSE(mock_timer_->IsRunning());
  media_sink_service_->SetDnsSdRegistryForTest(&test_dns_sd_registry_);
  media_sink_service_->Start();
  EXPECT_TRUE(mock_timer_->IsRunning());

  EXPECT_CALL(test_dns_sd_registry_, RemoveObserver(media_sink_service_.get()));
  EXPECT_CALL(test_dns_sd_registry_, UnregisterDnsSdListener(_));
  media_sink_service_->Stop();

  mock_timer_ =
      new base::MockTimer(true /*retain_user_task*/, false /*is_repeating*/);
  media_sink_service_->SetTimerForTest(base::WrapUnique(mock_timer_));
  media_sink_service_->SetDnsSdRegistryForTest(&test_dns_sd_registry_);
  media_sink_service_->Start();
  EXPECT_TRUE(mock_timer_->IsRunning());
}

TEST_F(CastMediaSinkServiceTest, TestMultipleStartAndStop) {
  EXPECT_CALL(test_dns_sd_registry_, AddObserver(media_sink_service_.get()));
  EXPECT_CALL(test_dns_sd_registry_, RegisterDnsSdListener(_));
  media_sink_service_->SetDnsSdRegistryForTest(&test_dns_sd_registry_);
  media_sink_service_->Start();
  media_sink_service_->Start();
  EXPECT_TRUE(mock_timer_->IsRunning());

  EXPECT_CALL(test_dns_sd_registry_, RemoveObserver(media_sink_service_.get()));
  EXPECT_CALL(test_dns_sd_registry_, UnregisterDnsSdListener(_));
  media_sink_service_->Stop();
  media_sink_service_->Stop();
}

TEST_F(CastMediaSinkServiceTest, TestOnChannelOpenedOnIOThread) {
  DnsSdService service = CreateDnsService(1);
  cast_channel::MockCastSocket socket;
  socket.set_id(1);

  media_sink_service_->services_pending_resolution_.push_back(service);
  media_sink_service_->OnChannelOpenedOnIOThread(service, &socket);
  // Invoke CastMediaSinkService::OnChannelOpenedOnUIThread on the UI thread.
  base::RunLoop().RunUntilIdle();

  // Verify sink content
  EXPECT_EQ(size_t(1), media_sink_service_->current_sinks_.size());
  for (const auto& sink_it : media_sink_service_->current_sinks_)
    VerifyMediaSinkInternal(sink_it, service, 1, false);
}

TEST_F(CastMediaSinkServiceTest, TestMultipleOnChannelOpenedOnIOThread) {
  DnsSdService service1 = CreateDnsService(1);
  DnsSdService service2 = CreateDnsService(2);
  DnsSdService service3 = CreateDnsService(3);

  cast_channel::MockCastSocket socket2;
  socket2.set_id(2);
  cast_channel::MockCastSocket socket3;
  socket3.set_id(3);

  // Current round of Dns discovery finds service1 and service 2.
  media_sink_service_->services_pending_resolution_.push_back(service1);
  media_sink_service_->services_pending_resolution_.push_back(service2);
  // Fail to open channel 1.
  media_sink_service_->OnChannelOpenedOnIOThread(service2, &socket2);
  media_sink_service_->OnChannelOpenedOnIOThread(service3, &socket3);
  // Invoke CastMediaSinkService::OnChannelOpenedOnUIThread on the UI thread.
  base::RunLoop().RunUntilIdle();

  // Verify sink content
  EXPECT_EQ(size_t(1), media_sink_service_->current_sinks_.size());
  for (const auto& sink_it : media_sink_service_->current_sinks_)
    VerifyMediaSinkInternal(sink_it, service2, 2, false);
}

TEST_F(CastMediaSinkServiceTest, TestOnDnsSdEvent) {
  DnsSdService service1 = CreateDnsService(1);
  DnsSdService service2 = CreateDnsService(2);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);
  net::IPEndPoint ip_endpoint2 = CreateIPEndPoint(2);

  // Add dns services.
  DnsSdRegistry::DnsSdServiceList service_list{service1, service2};

  cast_channel::MockCastSocket::MockOnOpenCallback callback1;
  cast_channel::MockCastSocket::MockOnOpenCallback callback2;
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .WillOnce(DoAll(SaveArg<2>(&callback1), Return(1)));
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _))
      .WillOnce(DoAll(SaveArg<2>(&callback2), Return(2)));

  // Invoke CastSocketService::OpenSocket on the IO thread.
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list);
  base::RunLoop().RunUntilIdle();

  cast_channel::MockCastSocket socket1;
  socket1.set_id(1);
  cast_channel::MockCastSocket socket2;
  socket2.set_id(2);

  callback1.Run(&socket1);
  callback2.Run(&socket2);

  // Invoke CastMediaSinkService::OnChannelOpenedOnUIThread on the UI thread.
  base::RunLoop().RunUntilIdle();

  // Verify sink content
  std::vector<MediaSinkInternal> current_sinks;
  EXPECT_CALL(mock_sink_discovered_cb_, Run(_))
      .WillOnce(SaveArg<0>(&current_sinks));
  mock_timer_->Fire();
  EXPECT_EQ(size_t(2), current_sinks.size());
}

TEST_F(CastMediaSinkServiceTest, TestMultipleOnDnsSdEvent) {
  DnsSdService service1 = CreateDnsService(1);
  DnsSdService service2 = CreateDnsService(2);
  DnsSdService service3 = CreateDnsService(3);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);
  net::IPEndPoint ip_endpoint2 = CreateIPEndPoint(2);
  net::IPEndPoint ip_endpoint3 = CreateIPEndPoint(3);

  // 1st round finds service 1 & 2.
  DnsSdRegistry::DnsSdServiceList service_list1{service1, service2};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list1);

  cast_channel::MockCastSocket::MockOnOpenCallback callback1;
  cast_channel::MockCastSocket::MockOnOpenCallback callback2;
  cast_channel::MockCastSocket::MockOnOpenCallback callback3;
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .WillOnce(DoAll(SaveArg<2>(&callback1), Return(1)));
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _))
      .WillOnce(DoAll(SaveArg<2>(&callback2), Return(2)));
  base::RunLoop().RunUntilIdle();

  // Channel 2 opened.
  cast_channel::MockCastSocket socket2;
  socket2.set_id(2);
  callback2.Run(&socket2);
  base::RunLoop().RunUntilIdle();

  // 2nd round finds service 2 & 3.
  DnsSdRegistry::DnsSdServiceList service_list2{service2, service3};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list2);

  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _))
      .WillOnce(DoAll(SaveArg<2>(&callback2), Return(4)));
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint3, _, _, _))
      .WillOnce(DoAll(SaveArg<2>(&callback3), Return(3)));
  base::RunLoop().RunUntilIdle();

  // Channel 1 and 3 opened.
  cast_channel::MockCastSocket socket1;
  cast_channel::MockCastSocket socket3;
  socket1.set_id(1);
  socket1.SetErrorState(cast_channel::ChannelError::CONNECT_ERROR);
  socket3.set_id(3);
  callback1.Run(&socket1);
  callback3.Run(&socket3);
  base::RunLoop().RunUntilIdle();

  // |current_sinks| should hold |service2| and |service3| since only |service1|
  // had an error.
  std::vector<MediaSinkInternal> current_sinks;
  EXPECT_CALL(mock_sink_discovered_cb_, Run(_))
      .WillOnce(SaveArg<0>(&current_sinks));
  mock_timer_->Fire();
  EXPECT_EQ(size_t(2), current_sinks.size());
  VerifyMediaSinkInternal(current_sinks[0], service2, 2, false);
  VerifyMediaSinkInternal(current_sinks[1], service3, 3, false);
}

TEST_F(CastMediaSinkServiceTest, TestTimer) {
  DnsSdService service1 = CreateDnsService(1);
  DnsSdService service2 = CreateDnsService(2);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);
  net::IPEndPoint ip_endpoint2 = CreateIPEndPoint(2);

  EXPECT_FALSE(mock_timer_->IsRunning());
  // finds service 1 & 2.
  DnsSdRegistry::DnsSdServiceList service_list1{service1, service2};
  media_sink_service_->OnDnsSdEvent(CastMediaSinkService::kCastServiceType,
                                    service_list1);

  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _));
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _));
  base::RunLoop().RunUntilIdle();

  // Channel 2 is opened.
  media_sink_service_->OnChannelOpenedOnUIThread(service2, 2, false);

  std::vector<MediaSinkInternal> sinks;
  EXPECT_CALL(mock_sink_discovered_cb_, Run(_)).WillOnce(SaveArg<0>(&sinks));

  // Fire timer.
  mock_timer_->Fire();
  EXPECT_EQ(size_t(1), sinks.size());
  VerifyMediaSinkInternal(sinks[0], service2, 2, false);

  EXPECT_FALSE(mock_timer_->IsRunning());
  // Channel 1 is opened and timer is restarted.
  media_sink_service_->OnChannelOpenedOnUIThread(service1, 1, false);
  EXPECT_TRUE(mock_timer_->IsRunning());
}

TEST_F(CastMediaSinkServiceTest, CacheServicesForKnownNetwork) {
  fake_network_info_ = fake_ethernet_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  content::RunAllBlockingPoolTasksUntilIdle();

  DnsSdService service1 = CreateDnsService(1);
  DnsSdService service2 = CreateDnsService(2);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);
  net::IPEndPoint ip_endpoint2 = CreateIPEndPoint(2);

  DnsSdRegistry::DnsSdServiceList service_list1{service1, service2};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list1);

  // Resolution will succeed for both services.
  cast_channel::CastSocket::Observer* observer;
  cast_channel::MockCastSocket socket1;
  cast_channel::MockCastSocket socket2;
  socket1.SetIPEndpoint(ip_endpoint1);
  socket1.set_id(1);
  socket2.SetIPEndpoint(ip_endpoint2);
  socket2.set_id(2);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .WillOnce(DoAll(SaveArg<3>(&observer),
                      Invoke([&socket1](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        content::BrowserThread::PostTask(
                            content::BrowserThread::IO, FROM_HERE,
                            base::Bind(std::move(open_cb), &socket1));
                      }),
                      Return(1)));
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _))
      .WillOnce(DoAll(Invoke([&socket2](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        content::BrowserThread::PostTask(
                            content::BrowserThread::IO, FROM_HERE,
                            base::Bind(std::move(open_cb), &socket2));
                      }),
                      Return(2)));
  content::RunAllBlockingPoolTasksUntilIdle();

  // Connect to a new network with different services.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  fake_network_info_ = fake_wifi_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  content::RunAllBlockingPoolTasksUntilIdle();
  observer->OnError(socket1, cast_channel::ChannelError::CAST_SOCKET_ERROR);
  observer->OnError(socket2, cast_channel::ChannelError::CAST_SOCKET_ERROR);

  DnsSdService service3 = CreateDnsService(3);
  net::IPEndPoint ip_endpoint3 = CreateIPEndPoint(3);

  DnsSdRegistry::DnsSdServiceList service_list2{service3};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list2);

  cast_channel::MockCastSocket socket3;
  socket3.SetIPEndpoint(ip_endpoint3);
  socket3.set_id(3);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint3, _, _, _))
      .WillOnce(DoAll(Invoke([&socket3](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        content::BrowserThread::PostTask(
                            content::BrowserThread::IO, FROM_HERE,
                            base::Bind(std::move(open_cb), &socket3));
                      }),
                      Return(3)));
  content::RunAllBlockingPoolTasksUntilIdle();

  // Reconnecting to the previous ethernet network should restore the same
  // services from the cache and attempt to resolve them.
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

TEST_F(CastMediaSinkServiceTest, CacheContainsOnlyResolvedServices) {
  fake_network_info_ = fake_ethernet_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  content::RunAllBlockingPoolTasksUntilIdle();

  DnsSdService service1 = CreateDnsService(1);
  DnsSdService service2 = CreateDnsService(2);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);
  net::IPEndPoint ip_endpoint2 = CreateIPEndPoint(2);

  DnsSdRegistry::DnsSdServiceList service_list1{service1, service2};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list1);

  // Resolution will fail for |service2|.
  cast_channel::CastSocket::Observer* observer;
  cast_channel::MockCastSocket socket1;
  cast_channel::MockCastSocket socket2;
  socket1.SetIPEndpoint(ip_endpoint1);
  socket1.set_id(1);
  socket2.SetErrorState(cast_channel::ChannelError::CONNECT_ERROR);
  socket2.set_id(2);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .WillOnce(DoAll(SaveArg<3>(&observer),
                      Invoke([&socket1](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        content::BrowserThread::PostTask(
                            content::BrowserThread::IO, FROM_HERE,
                            base::Bind(std::move(open_cb), &socket1));
                      }),
                      Return(1)));
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _))
      .WillOnce(DoAll(Invoke([&socket2](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        content::BrowserThread::PostTask(
                            content::BrowserThread::IO, FROM_HERE,
                            base::Bind(std::move(open_cb), &socket2));
                      }),
                      Return(2)));
  content::RunAllBlockingPoolTasksUntilIdle();

  // Connect to a new network with different services.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  fake_network_info_ = fake_wifi_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  content::RunAllBlockingPoolTasksUntilIdle();
  observer->OnError(socket1, cast_channel::ChannelError::CAST_SOCKET_ERROR);

  DnsSdService service3 = CreateDnsService(3);
  net::IPEndPoint ip_endpoint3 = CreateIPEndPoint(3);

  DnsSdRegistry::DnsSdServiceList service_list2{service3};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list2);

  cast_channel::MockCastSocket socket3;
  socket3.SetIPEndpoint(ip_endpoint3);
  socket3.set_id(3);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint3, _, _, _))
      .WillOnce(DoAll(Invoke([&socket3](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        content::BrowserThread::PostTask(
                            content::BrowserThread::IO, FROM_HERE,
                            base::Bind(std::move(open_cb), &socket3));
                      }),
                      Return(3)));
  content::RunAllBlockingPoolTasksUntilIdle();

  // Reconnecting to the previous ethernet network should restore only
  // |service1|, since |service2| failed to resolve.
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

TEST_F(CastMediaSinkServiceTest, CacheUpdatedOnChannelError) {
  fake_network_info_ = fake_ethernet_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  content::RunAllBlockingPoolTasksUntilIdle();

  DnsSdService service1 = CreateDnsService(1);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);

  DnsSdRegistry::DnsSdServiceList service_list1{service1};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list1);

  // Resolve |service1| but then raise a channel error.  This should remove it
  // from the cached services for the ethernet network.
  cast_channel::CastSocket::Observer* observer;
  cast_channel::MockCastSocket socket1;
  socket1.SetIPEndpoint(ip_endpoint1);
  socket1.set_id(1);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .WillOnce(DoAll(SaveArg<3>(&observer),
                      Invoke([&socket1](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        content::BrowserThread::PostTask(
                            content::BrowserThread::IO, FROM_HERE,
                            base::Bind(std::move(open_cb), &socket1));
                      }),
                      Return(1)));
  content::RunAllBlockingPoolTasksUntilIdle();
  observer->OnError(socket1, cast_channel::ChannelError::CAST_SOCKET_ERROR);

  // Connect to a new network with different services.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  fake_network_info_ = fake_wifi_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  content::RunAllBlockingPoolTasksUntilIdle();

  DnsSdService service2 = CreateDnsService(2);
  net::IPEndPoint ip_endpoint2 = CreateIPEndPoint(2);

  DnsSdRegistry::DnsSdServiceList service_list2{service2};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list2);

  cast_channel::MockCastSocket socket2;
  socket2.SetIPEndpoint(ip_endpoint2);
  socket2.set_id(2);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _))
      .WillOnce(DoAll(Invoke([&socket2](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        content::BrowserThread::PostTask(
                            content::BrowserThread::IO, FROM_HERE,
                            base::Bind(std::move(open_cb), &socket2));
                      }),
                      Return(2)));
  content::RunAllBlockingPoolTasksUntilIdle();

  // Reconnecting to the previous ethernet network should not restore any
  // services since the only service to resolve successfully, |service1|, later
  // had a channel error.
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

TEST_F(CastMediaSinkServiceTest, CacheFirstDiscoveryConflict) {
  fake_network_info_ = fake_ethernet_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  content::RunAllBlockingPoolTasksUntilIdle();

  DnsSdService service1 = CreateDnsService(1);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);

  DnsSdRegistry::DnsSdServiceList service_list1{service1};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list1);

  // Resolve |service1| so it will be cached.
  cast_channel::CastSocket::Observer* observer;
  cast_channel::MockCastSocket socket1;
  socket1.SetIPEndpoint(ip_endpoint1);
  socket1.set_id(1);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .WillOnce(DoAll(SaveArg<3>(&observer),
                      Invoke([&socket1](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        content::BrowserThread::PostTask(
                            content::BrowserThread::IO, FROM_HERE,
                            base::Bind(std::move(open_cb), &socket1));
                      }),
                      Return(1)));
  content::RunAllBlockingPoolTasksUntilIdle();

  // Connect to a new network with different services.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();
  observer->OnError(socket1, cast_channel::ChannelError::CAST_SOCKET_ERROR);

  fake_network_info_ = fake_wifi_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  content::RunAllBlockingPoolTasksUntilIdle();

  DnsSdService service2 = CreateDnsService(2);
  net::IPEndPoint ip_endpoint2 = CreateIPEndPoint(2);

  DnsSdRegistry::DnsSdServiceList service_list2{service2};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list2);

  cast_channel::MockCastSocket socket2;
  socket2.SetIPEndpoint(ip_endpoint2);
  socket2.set_id(2);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _))
      .WillOnce(DoAll(Invoke([&socket2](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        content::BrowserThread::PostTask(
                            content::BrowserThread::IO, FROM_HERE,
                            base::Bind(std::move(open_cb), &socket2));
                      }),
                      Return(2)));
  content::RunAllBlockingPoolTasksUntilIdle();

  // Reconnecting to the previous ethernet network should restore |service1|
  // with the original IP address.  Discovery will then report a new IP for
  // |service1|, which is correct.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  cast_channel::MockCastSocket::MockOnOpenCallback open_cb;
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .WillOnce(DoAll(SaveArg<2>(&open_cb), Return(3)));
  fake_network_info_ = fake_ethernet_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  content::RunAllBlockingPoolTasksUntilIdle();

  // Discovery reports the new IP before the first cast socket has failed.
  net::IPEndPoint ip_endpoint3 = CreateIPEndPoint(3);
  service1.ip_address = ip_endpoint3.address().ToString();
  service1.service_host_port = net::HostPortPair::FromIPEndPoint(ip_endpoint3);
  DnsSdRegistry::DnsSdServiceList service_list3{service1};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list3);

  // When discovery reports the new IP, immediately try to connect to it and
  // ignore the result of the cached service's cast socket connection, even if
  // it succeeds.
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint3, _, _, _));
  cast_channel::MockCastSocket socket3;
  socket2.SetIPEndpoint(ip_endpoint3);
  socket2.set_id(3);
  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                   base::Bind(std::move(open_cb), &socket3));
  content::RunAllBlockingPoolTasksUntilIdle();
}

TEST_F(CastMediaSinkServiceTest, DiscoveryFirstCacheConflict) {
  fake_network_info_ = fake_ethernet_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  content::RunAllBlockingPoolTasksUntilIdle();

  DnsSdService service1 = CreateDnsService(1);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);

  DnsSdRegistry::DnsSdServiceList service_list1{service1};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list1);

  // Resolve |service1| so it will be cached.
  cast_channel::CastSocket::Observer* observer;
  cast_channel::MockCastSocket socket1;
  socket1.SetIPEndpoint(ip_endpoint1);
  socket1.set_id(1);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .WillOnce(DoAll(SaveArg<3>(&observer),
                      Invoke([&socket1](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        content::BrowserThread::PostTask(
                            content::BrowserThread::IO, FROM_HERE,
                            base::Bind(std::move(open_cb), &socket1));
                      }),
                      Return(1)));
  content::RunAllBlockingPoolTasksUntilIdle();

  // Connect to a new network with different services.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();
  observer->OnError(socket1, cast_channel::ChannelError::CAST_SOCKET_ERROR);

  fake_network_info_ = fake_wifi_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  content::RunAllBlockingPoolTasksUntilIdle();

  DnsSdService service2 = CreateDnsService(2);
  net::IPEndPoint ip_endpoint2 = CreateIPEndPoint(2);

  DnsSdRegistry::DnsSdServiceList service_list2{service2};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list2);

  cast_channel::MockCastSocket socket2;
  socket2.SetIPEndpoint(ip_endpoint2);
  socket2.set_id(2);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _))
      .WillOnce(DoAll(Invoke([&socket2](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        content::BrowserThread::PostTask(
                            content::BrowserThread::IO, FROM_HERE,
                            base::Bind(std::move(open_cb), &socket2));
                      }),
                      Return(2)));
  content::RunAllBlockingPoolTasksUntilIdle();

  // Discovery reports the new IP before the cache has a chance to see the new
  // network ID.
  net::IPEndPoint ip_endpoint3 = CreateIPEndPoint(3);
  service1.ip_address = ip_endpoint3.address().ToString();
  service1.service_host_port = net::HostPortPair::FromIPEndPoint(ip_endpoint3);
  DnsSdRegistry::DnsSdServiceList service_list3{service1};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list3);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint3, _, _, _));

  // Reconnecting to the previous ethernet network should attempt to restore
  // |service1| with the original IP address but fail because discovery already
  // returned a different definition of |service1|.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .Times(0);
  fake_network_info_ = fake_ethernet_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  content::RunAllBlockingPoolTasksUntilIdle();
}

TEST_F(CastMediaSinkServiceTest, UnknownNetworkNoCache) {
  // Without any network notification here, network ID will remain __unknown__
  // and the cache shouldn't save any of these services.
  DnsSdService service1 = CreateDnsService(1);
  DnsSdService service2 = CreateDnsService(2);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);
  net::IPEndPoint ip_endpoint2 = CreateIPEndPoint(2);

  DnsSdRegistry::DnsSdServiceList service_list1{service1, service2};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list1);

  // Resolution will succeed for both services.
  cast_channel::CastSocket::Observer* observer;
  cast_channel::MockCastSocket socket1;
  cast_channel::MockCastSocket socket2;
  socket1.SetIPEndpoint(ip_endpoint1);
  socket1.set_id(1);
  socket2.SetIPEndpoint(ip_endpoint2);
  socket2.set_id(2);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .WillOnce(DoAll(SaveArg<3>(&observer),
                      Invoke([&socket1](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        content::BrowserThread::PostTask(
                            content::BrowserThread::IO, FROM_HERE,
                            base::Bind(std::move(open_cb), &socket1));
                      }),
                      Return(1)));
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _))
      .WillOnce(DoAll(Invoke([&socket2](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        content::BrowserThread::PostTask(
                            content::BrowserThread::IO, FROM_HERE,
                            base::Bind(std::move(open_cb), &socket2));
                      }),
                      Return(2)));
  content::RunAllBlockingPoolTasksUntilIdle();

  // Connect to a new network with different services.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  fake_network_info_ = fake_wifi_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  content::RunAllBlockingPoolTasksUntilIdle();
  observer->OnError(socket1, cast_channel::ChannelError::CAST_SOCKET_ERROR);
  observer->OnError(socket2, cast_channel::ChannelError::CAST_SOCKET_ERROR);

  DnsSdService service3 = CreateDnsService(3);
  net::IPEndPoint ip_endpoint3 = CreateIPEndPoint(3);

  DnsSdRegistry::DnsSdServiceList service_list2{service3};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list2);

  cast_channel::MockCastSocket socket3;
  socket3.SetIPEndpoint(ip_endpoint3);
  socket3.set_id(3);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint3, _, _, _))
      .WillOnce(DoAll(Invoke([&socket3](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        content::BrowserThread::PostTask(
                            content::BrowserThread::IO, FROM_HERE,
                            base::Bind(std::move(open_cb), &socket3));
                      }),
                      Return(3)));
  content::RunAllBlockingPoolTasksUntilIdle();

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

TEST_F(CastMediaSinkServiceTest, DisconnectedNoCache) {
  // Discovery can report services while we think we're disconnected, but
  // nothing should be cached.
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  DnsSdService service1 = CreateDnsService(1);
  DnsSdService service2 = CreateDnsService(2);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);
  net::IPEndPoint ip_endpoint2 = CreateIPEndPoint(2);

  DnsSdRegistry::DnsSdServiceList service_list1{service1, service2};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list1);

  // Resolution will succeed for both services.
  cast_channel::CastSocket::Observer* observer;
  cast_channel::MockCastSocket socket1;
  cast_channel::MockCastSocket socket2;
  socket1.SetIPEndpoint(ip_endpoint1);
  socket1.set_id(1);
  socket2.SetIPEndpoint(ip_endpoint2);
  socket2.set_id(2);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .WillOnce(DoAll(SaveArg<3>(&observer),
                      Invoke([&socket1](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        content::BrowserThread::PostTask(
                            content::BrowserThread::IO, FROM_HERE,
                            base::Bind(std::move(open_cb), &socket1));
                      }),
                      Return(1)));
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint2, _, _, _))
      .WillOnce(DoAll(Invoke([&socket2](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        content::BrowserThread::PostTask(
                            content::BrowserThread::IO, FROM_HERE,
                            base::Bind(std::move(open_cb), &socket2));
                      }),
                      Return(2)));
  content::RunAllBlockingPoolTasksUntilIdle();

  // Connect to a new known network with different services.
  fake_network_info_ = fake_wifi_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  content::RunAllBlockingPoolTasksUntilIdle();
  observer->OnError(socket1, cast_channel::ChannelError::CAST_SOCKET_ERROR);
  observer->OnError(socket2, cast_channel::ChannelError::CAST_SOCKET_ERROR);

  DnsSdService service3 = CreateDnsService(3);
  net::IPEndPoint ip_endpoint3 = CreateIPEndPoint(3);

  DnsSdRegistry::DnsSdServiceList service_list2{service3};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list2);

  cast_channel::MockCastSocket socket3;
  socket3.SetIPEndpoint(ip_endpoint3);
  socket3.set_id(3);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint3, _, _, _))
      .WillOnce(DoAll(Invoke([&socket3](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        content::BrowserThread::PostTask(
                            content::BrowserThread::IO, FROM_HERE,
                            base::Bind(std::move(open_cb), &socket3));
                      }),
                      Return(3)));
  content::RunAllBlockingPoolTasksUntilIdle();

  // Connecting to a network whose ID resolves to __unknown__ shouldn't pull any
  // cache items from another unknown network.
  EXPECT_CALL(*mock_cast_socket_service_, OpenSocketInternal(_, _, _, _))
      .Times(0);
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();
}

TEST_F(CastMediaSinkServiceTest, InvalidIPWontGeneratePendingRequest) {
  fake_network_info_ = fake_ethernet_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  content::RunAllBlockingPoolTasksUntilIdle();

  DnsSdService service1 = CreateDnsService(1);
  net::IPEndPoint ip_endpoint1 = CreateIPEndPoint(1);

  DnsSdRegistry::DnsSdServiceList service_list1{service1};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list1);

  // Resolution will succeed for |service1|.
  cast_channel::CastSocket::Observer* observer;
  cast_channel::MockCastSocket socket1;
  socket1.SetIPEndpoint(ip_endpoint1);
  socket1.set_id(1);
  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _))
      .WillOnce(DoAll(SaveArg<3>(&observer),
                      Invoke([&socket1](const auto& ip_endpoint, auto* net_log,
                                        auto open_cb, auto* observer) {
                        content::BrowserThread::PostTask(
                            content::BrowserThread::IO, FROM_HERE,
                            base::Bind(std::move(open_cb), &socket1));
                      }),
                      Return(1)));
  content::RunAllBlockingPoolTasksUntilIdle();

  // Connect to a new network and clear |service1| from current services.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  fake_network_info_ = fake_wifi_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  content::RunAllBlockingPoolTasksUntilIdle();
  observer->OnError(socket1, cast_channel::ChannelError::CAST_SOCKET_ERROR);

  // "Discover" |service1| again with a bad IP address.  No cast socket should
  // be opened and we should not be left with any pending request.
  service1.ip_address = "not an ip address";
  DnsSdRegistry::DnsSdServiceList service_list2{service1};
  static_cast<DnsSdRegistry::DnsSdObserver*>(media_sink_service_.get())
      ->OnDnsSdEvent(CastMediaSinkService::kCastServiceType, service_list2);
  EXPECT_CALL(*mock_cast_socket_service_, OpenSocketInternal(_, _, _, _))
      .Times(0);
  content::RunAllBlockingPoolTasksUntilIdle();

  // Reconnecting to the previous ethernet network should restore the |service1|
  // with the original IP and no pending request should block this.
  fake_network_info_.clear();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  content::RunAllBlockingPoolTasksUntilIdle();

  EXPECT_CALL(*mock_cast_socket_service_,
              OpenSocketInternal(ip_endpoint1, _, _, _));
  fake_network_info_ = fake_ethernet_info_;
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  content::RunAllBlockingPoolTasksUntilIdle();
}

}  // namespace media_router
