// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/dial/dial_media_sink_service_impl.h"
#include "base/test/mock_callback.h"
#include "base/timer/mock_timer.h"
#include "chrome/browser/media/router/discovery/dial/dial_device_data.h"
#include "chrome/browser/media/router/discovery/dial/dial_registry.h"
#include "chrome/browser/media/router/test_helper.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::Return;

namespace {

GURL CreateAppURL(const net::IPAddress& ip_address,
                  const std::string& app_name) {
  std::string str_app_url = base::StringPrintf(
      "http://%s/apps/%s", ip_address.ToString().c_str(), app_name.c_str());
  return GURL(str_app_url);
}

}  // namespace

namespace media_router {

class TestDialRegistry : public DialRegistry {
 public:
  TestDialRegistry() {}
  ~TestDialRegistry() {}

  MOCK_METHOD1(RegisterObserver, void(DialRegistry::Observer* observer));
  MOCK_METHOD1(UnregisterObserver, void(DialRegistry::Observer* observer));

  MOCK_METHOD0(OnListenerAdded, void());
  MOCK_METHOD0(OnListenerRemoved, void());
};

class MockDialMediaSinkServiceObserver : public DialMediaSinkServiceObserver {
 public:
  MockDialMediaSinkServiceObserver() {}
  ~MockDialMediaSinkServiceObserver() = default;

  MOCK_METHOD1(OnDialSinkAdded, void(const MediaSinkInternal& sink));
};

class MockDeviceDescriptionService : public DeviceDescriptionService {
 public:
  MockDeviceDescriptionService(DeviceDescriptionParseSuccessCallback success_cb,
                               DeviceDescriptionParseErrorCallback error_cb)
      : DeviceDescriptionService(success_cb, error_cb) {}
  ~MockDeviceDescriptionService() override {}

  MOCK_METHOD2(GetDeviceDescriptions,
               void(const std::vector<DialDeviceData>& devices,
                    net::URLRequestContextGetter* request_context));
};

class MockDialAppDiscoveryService : public DialAppDiscoveryService {
 public:
  MockDialAppDiscoveryService(const DialAppInfoParseSuccessCallback& success_cb,
                              const DialAppInfoParseErrorCallback& error_cb)
      : DialAppDiscoveryService(success_cb, error_cb) {}
  ~MockDialAppDiscoveryService() override {}

  MOCK_METHOD2(FetchDialAppInfo,
               void(const GURL& app_url,
                    net::URLRequestContextGetter* request_context));
};

class DialMediaSinkServiceImplTest : public ::testing::Test {
 public:
  DialMediaSinkServiceImplTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        media_sink_service_(
            new DialMediaSinkServiceImpl(mock_sink_discovered_cb_.Get(),
                                         mock_available_sinks_updated_cb_.Get(),
                                         profile_.GetRequestContext())) {}

  void SetUp() override {
    media_sink_service_->SetDialRegistryForTest(&test_dial_registry_);

    auto mock_description_service =
        base::MakeUnique<MockDeviceDescriptionService>(mock_success_cb_.Get(),
                                                       mock_error_cb_.Get());
    mock_description_service_ = mock_description_service.get();
    media_sink_service_->SetDescriptionServiceForTest(
        std::move(mock_description_service));

    mock_timer_ =
        new base::MockTimer(true /*retain_user_task*/, false /*is_repeating*/);
    media_sink_service_->SetTimerForTest(base::WrapUnique(mock_timer_));

    auto mock_app_discovery_service =
        base::MakeUnique<MockDialAppDiscoveryService>(
            base::Bind(&DialMediaSinkServiceImpl::OnAppInfoAvailable,
                       base::Unretained(media_sink_service_.get())),
            base::Bind(&DialMediaSinkServiceImpl::OnAppInfoError,
                       base::Unretained(media_sink_service_.get())));
    mock_app_discovery_service_ = mock_app_discovery_service.get();
    media_sink_service_->SetAppDiscoveryServiceForTest(
        std::move(mock_app_discovery_service));
  }

  void TearDown() override {
  }

 protected:
  const content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;

  base::MockCallback<MediaSinkService::OnSinksDiscoveredCallback>
      mock_sink_discovered_cb_;
  base::MockCallback<DialMediaSinkServiceImpl::OnAvailableSinksUpdatedCallback>
      mock_available_sinks_updated_cb_;
  base::MockCallback<
      MockDeviceDescriptionService::DeviceDescriptionParseSuccessCallback>
      mock_success_cb_;
  base::MockCallback<
      MockDeviceDescriptionService::DeviceDescriptionParseErrorCallback>
      mock_error_cb_;

  TestDialRegistry test_dial_registry_;
  MockDeviceDescriptionService* mock_description_service_;
  MockDialAppDiscoveryService* mock_app_discovery_service_;
  base::MockTimer* mock_timer_;

  std::unique_ptr<DialMediaSinkServiceImpl> media_sink_service_;

  DISALLOW_COPY_AND_ASSIGN(DialMediaSinkServiceImplTest);
};

TEST_F(DialMediaSinkServiceImplTest, TestOnDeviceDescriptionAvailable) {
  DialDeviceData device_data("first", GURL("http://127.0.0.1/dd.xml"),
                             base::Time::Now());
  ParsedDialDeviceDescription device_description;
  device_description.model_name = "model name";
  device_description.friendly_name = "friendly name";
  device_description.app_url = GURL("http://192.168.1.1/apps");
  device_description.unique_id = "unique id";

  media_sink_service_->OnDeviceDescriptionAvailable(device_data,
                                                    device_description);
  EXPECT_TRUE(media_sink_service_->current_sinks_.empty());

  std::vector<DialDeviceData> device_list = {device_data};
  EXPECT_CALL(*mock_description_service_,
              GetDeviceDescriptions(device_list, _));

  media_sink_service_->OnDialDeviceEvent(device_list);
  media_sink_service_->OnDeviceDescriptionAvailable(device_data,
                                                    device_description);

  EXPECT_EQ(1u, media_sink_service_->current_sinks_.size());
}

TEST_F(DialMediaSinkServiceImplTest, TestTimer) {
  DialDeviceData device_data("first", GURL("http://127.0.0.1/dd.xml"),
                             base::Time::Now());
  ParsedDialDeviceDescription device_description;
  device_description.model_name = "model name";
  device_description.friendly_name = "friendly name";
  device_description.app_url = GURL("http://192.168.1.1/apps");
  device_description.unique_id = "unique id";

  std::vector<DialDeviceData> device_list = {device_data};
  EXPECT_CALL(*mock_description_service_,
              GetDeviceDescriptions(device_list, _));

  EXPECT_FALSE(mock_timer_->IsRunning());
  media_sink_service_->OnDialDeviceEvent(device_list);
  media_sink_service_->OnDeviceDescriptionAvailable(device_data,
                                                    device_description);
  EXPECT_TRUE(mock_timer_->IsRunning());

  EXPECT_CALL(mock_sink_discovered_cb_, Run(_));
  mock_timer_->Fire();

  EXPECT_FALSE(mock_timer_->IsRunning());
  device_description.app_url = GURL("http://192.168.1.11/apps");
  media_sink_service_->OnDeviceDescriptionAvailable(device_data,
                                                    device_description);
  EXPECT_TRUE(mock_timer_->IsRunning());
}

TEST_F(DialMediaSinkServiceImplTest, TestRestartAfterStop) {
  EXPECT_CALL(test_dial_registry_, RegisterObserver(media_sink_service_.get()))
      .Times(2);
  EXPECT_CALL(test_dial_registry_, OnListenerAdded()).Times(2);
  media_sink_service_->Start();
  EXPECT_TRUE(mock_timer_->IsRunning());

  EXPECT_CALL(test_dial_registry_,
              UnregisterObserver(media_sink_service_.get()));
  EXPECT_CALL(test_dial_registry_, OnListenerRemoved());
  media_sink_service_->Stop();

  mock_timer_ =
      new base::MockTimer(true /*retain_user_task*/, false /*is_repeating*/);
  media_sink_service_->SetTimerForTest(base::WrapUnique(mock_timer_));
  media_sink_service_->Start();
  EXPECT_TRUE(mock_timer_->IsRunning());

  EXPECT_CALL(test_dial_registry_,
              UnregisterObserver(media_sink_service_.get()));
  EXPECT_CALL(test_dial_registry_, OnListenerRemoved());
}

TEST_F(DialMediaSinkServiceImplTest, DialMediaSinkServiceObserver) {
  MockDialMediaSinkServiceObserver observer;
  media_sink_service_->SetObserver(&observer);

  DialDeviceData device_data1("first", GURL("http://127.0.0.1/dd.xml"),
                              base::Time::Now());
  ParsedDialDeviceDescription device_description1;
  device_description1.model_name = "model name";
  device_description1.friendly_name = "friendly name";
  device_description1.app_url = GURL("http://192.168.1.1/apps");
  device_description1.unique_id = "unique id 1";

  DialDeviceData device_data2("second", GURL("http://127.0.0.2/dd.xml"),
                              base::Time::Now());
  ParsedDialDeviceDescription device_description2;
  device_description2.model_name = "model name";
  device_description2.friendly_name = "friendly name";
  device_description2.app_url = GURL("http://192.168.1.2/apps");
  device_description2.unique_id = "unique id 2";

  std::vector<DialDeviceData> device_list = {device_data1, device_data2};
  EXPECT_CALL(*mock_description_service_,
              GetDeviceDescriptions(device_list, _));
  media_sink_service_->OnDialDeviceEvent(device_list);

  EXPECT_CALL(observer, OnDialSinkAdded(_)).Times(1);
  media_sink_service_->OnDeviceDescriptionAvailable(device_data1,
                                                    device_description1);

  EXPECT_CALL(observer, OnDialSinkAdded(_)).Times(1);
  media_sink_service_->OnDeviceDescriptionAvailable(device_data2,
                                                    device_description2);

  // OnUserGesture will "re-sync" all existing sinks to observer.
  EXPECT_CALL(observer, OnDialSinkAdded(_)).Times(2);
  media_sink_service_->OnUserGesture();

  media_sink_service_->ClearObserver();
}

TEST_F(DialMediaSinkServiceImplTest,
       TestStartStopMonitoringAvailableSinksForApp) {
  MediaSinkInternal dial_sink = CreateDialSink(1);
  EXPECT_CALL(*mock_app_discovery_service_,
              FetchDialAppInfo(GURL("http://192.168.0.101/apps/YouTube"), _))
      .Times(1);
  media_sink_service_->current_sinks_.insert(dial_sink);
  media_sink_service_->StartMonitoringAvailableSinksForApp("YouTube");
  media_sink_service_->StartMonitoringAvailableSinksForApp("YouTube");
  EXPECT_EQ(1u, media_sink_service_->app_name_sink_id_map_.size());

  media_sink_service_->StopMonitoringAvailableSinksForApp("YouTube");
  EXPECT_TRUE(media_sink_service_->app_name_sink_id_map_.empty());
}

TEST_F(DialMediaSinkServiceImplTest,
       TestOnDialAppInfoAvailableNoStartMonitoring) {
  MediaSinkInternal dial_sink = CreateDialSink(1);
  GURL app_url("http://192.168.0.101/apps/YouTube");
  ParsedDialAppInfo parsed_app_info{"YouTube", DialAppStatus::AVAILABLE};

  EXPECT_CALL(mock_available_sinks_updated_cb_, Run(_, _)).Times(0);
  media_sink_service_->current_sinks_.insert(dial_sink);
  media_sink_service_->OnAppInfoAvailable(app_url, parsed_app_info);
}

TEST_F(DialMediaSinkServiceImplTest, TestOnDialAppInfoAvailableNoSink) {
  GURL app_url("http://192.168.0.101/apps/YouTube");
  ParsedDialAppInfo parsed_app_info{"YouTube", DialAppStatus::AVAILABLE};

  EXPECT_CALL(mock_available_sinks_updated_cb_, Run("YouTube", _)).Times(0);
  media_sink_service_->StartMonitoringAvailableSinksForApp("YouTube");
  media_sink_service_->OnAppInfoAvailable(app_url, parsed_app_info);
}

TEST_F(DialMediaSinkServiceImplTest, TestOnDialAppInfoAvailableSinksAdded) {
  MediaSinkInternal dial_sink1 = CreateDialSink(1);
  MediaSinkInternal dial_sink2 = CreateDialSink(2);

  media_sink_service_->current_sinks_.insert(dial_sink1);
  media_sink_service_->current_sinks_.insert(dial_sink2);

  GURL app_url1 = CreateAppURL(dial_sink1.dial_data().ip_address, "YouTube");
  GURL app_url2 = CreateAppURL(dial_sink1.dial_data().ip_address, "Netflix");
  GURL app_url3 = CreateAppURL(dial_sink2.dial_data().ip_address, "YouTube");
  GURL app_url4 = CreateAppURL(dial_sink2.dial_data().ip_address, "Netflix");

  ParsedDialAppInfo app_info_youtube{"YouTube", DialAppStatus::AVAILABLE};
  ParsedDialAppInfo app_info_netflix{"Netflix", DialAppStatus::AVAILABLE};

  EXPECT_CALL(*mock_app_discovery_service_, FetchDialAppInfo(_, _)).Times(4);
  media_sink_service_->StartMonitoringAvailableSinksForApp("YouTube");
  media_sink_service_->StartMonitoringAvailableSinksForApp("Netflix");

  std::vector<MediaSink> expected_sinks = {dial_sink1.sink()};
  EXPECT_CALL(mock_available_sinks_updated_cb_, Run("YouTube", expected_sinks));
  media_sink_service_->OnAppInfoAvailable(app_url1, app_info_youtube);

  expected_sinks = {dial_sink1.sink(), dial_sink2.sink()};
  EXPECT_CALL(mock_available_sinks_updated_cb_, Run("YouTube", expected_sinks));
  media_sink_service_->OnAppInfoAvailable(app_url3, app_info_youtube);

  expected_sinks = {dial_sink2.sink()};
  EXPECT_CALL(mock_available_sinks_updated_cb_, Run("Netflix", expected_sinks));
  media_sink_service_->OnAppInfoAvailable(app_url4, app_info_netflix);
}

TEST_F(DialMediaSinkServiceImplTest, TestOnDialAppInfoAvailableSinksRemoved) {
  MediaSinkInternal dial_sink1 = CreateDialSink(1);
  GURL app_url1 = CreateAppURL(dial_sink1.dial_data().ip_address, "YouTube");
  ParsedDialAppInfo parsed_app_info{"YouTube", DialAppStatus::AVAILABLE};

  EXPECT_CALL(*mock_app_discovery_service_, FetchDialAppInfo(_, _));
  media_sink_service_->current_sinks_.insert(dial_sink1);
  media_sink_service_->StartMonitoringAvailableSinksForApp("YouTube");

  std::vector<MediaSink> expected_sinks = {dial_sink1.sink()};
  EXPECT_CALL(mock_available_sinks_updated_cb_, Run("YouTube", expected_sinks));
  media_sink_service_->OnAppInfoAvailable(app_url1, parsed_app_info);

  EXPECT_CALL(mock_available_sinks_updated_cb_,
              Run("YouTube", std::vector<MediaSink>()));
  media_sink_service_->OnAppInfoError(app_url1, "");

  parsed_app_info.app_status = DialAppStatus::UNAVAILABLE;
  EXPECT_CALL(mock_available_sinks_updated_cb_, Run(_, _)).Times(0);
  media_sink_service_->OnAppInfoAvailable(app_url1, parsed_app_info);
}

TEST_F(DialMediaSinkServiceImplTest,
       TestOnDialAppInfoAvailableWithAlreadyAvailableSinks) {
  MediaSinkInternal dial_sink1 = CreateDialSink(1);
  GURL app_url1 = CreateAppURL(dial_sink1.dial_data().ip_address, "YouTube");
  ParsedDialAppInfo parsed_app_info{"YouTube", DialAppStatus::AVAILABLE};

  EXPECT_CALL(*mock_app_discovery_service_, FetchDialAppInfo(_, _));
  media_sink_service_->current_sinks_.insert(dial_sink1);
  media_sink_service_->StartMonitoringAvailableSinksForApp("YouTube");

  std::vector<MediaSink> expected_sinks = {dial_sink1.sink()};
  EXPECT_CALL(mock_available_sinks_updated_cb_, Run("YouTube", expected_sinks))
      .Times(1);
  media_sink_service_->OnAppInfoAvailable(app_url1, parsed_app_info);
  media_sink_service_->OnAppInfoAvailable(app_url1, parsed_app_info);
}

}  // namespace media_router
