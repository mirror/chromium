// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/dial/dial_media_sink_service_proxy.h"
#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "chrome/browser/media/router/discovery/dial/dial_media_sink_service_impl.h"
#include "chrome/browser/media/router/mock_media_router.h"
#include "chrome/browser/media/router/test_helper.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::InvokeWithoutArgs;
using ::testing::Return;

namespace media_router {

class MockDialMediaSinkServiceImpl : public DialMediaSinkServiceImpl {
 public:
  MockDialMediaSinkServiceImpl(
      const MediaSinkService::OnSinksDiscoveredCallback&
          sink_discovery_callback,
      const OnAvailableSinksUpdatedCallback& available_sinks_updated_cb,
      net::URLRequestContextGetter* request_context)
      : DialMediaSinkServiceImpl(sink_discovery_callback,
                                 available_sinks_updated_cb,
                                 request_context) {}
  ~MockDialMediaSinkServiceImpl() override {}

  MOCK_METHOD0(Start, void());
  MOCK_METHOD0(Stop, void());

  void OnSinksDiscovered() {
    sink_discovery_callback_.Run(std::vector<MediaSinkInternal>());
  }

  void OnAvailableSinksUpdated(const std::string& app_name,
                               const std::vector<MediaSink>& available_sinks) {
    available_sinks_updated_callback_.Run(app_name, available_sinks);
  }
};

class DialMediaSinkServiceProxyTest : public ::testing::Test {
 public:
  DialMediaSinkServiceProxyTest() {
    proxy_ = new DialMediaSinkServiceProxy(mock_sink_discovered_cb_.Get(),
                                           &profile_);
    auto mock_dial_media_sink_service =
        base::MakeUnique<MockDialMediaSinkServiceImpl>(
            base::Bind(&DialMediaSinkServiceProxy::OnSinksDiscoveredOnIOThread,
                       base::Unretained(proxy_.get())),
            base::Bind(
                &DialMediaSinkServiceProxy::OnAvailableSinksUpdatedOnIOThread,
                base::Unretained(proxy_.get())),
            profile_.GetRequestContext());
    mock_service_ = mock_dial_media_sink_service.get();
    proxy_->SetDialMediaSinkServiceForTest(
        std::move(mock_dial_media_sink_service));
  }

  void TearDown() override { base::RunLoop().RunUntilIdle(); }

 protected:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;

  base::MockCallback<MediaSinkService::OnSinksDiscoveredCallback>
      mock_sink_discovered_cb_;

  MockDialMediaSinkServiceImpl* mock_service_;
  scoped_refptr<DialMediaSinkServiceProxy> proxy_;

  DISALLOW_COPY_AND_ASSIGN(DialMediaSinkServiceProxyTest);
};

TEST_F(DialMediaSinkServiceProxyTest, TestStart) {
  EXPECT_CALL(*mock_service_, Start()).WillOnce(InvokeWithoutArgs([]() {
    EXPECT_TRUE(
        content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  }));

  proxy_->Start();
}

TEST_F(DialMediaSinkServiceProxyTest, TestStop) {
  EXPECT_CALL(*mock_service_, Stop()).WillOnce(InvokeWithoutArgs([]() {
    EXPECT_TRUE(
        content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  }));

  proxy_->Stop();
}

TEST_F(DialMediaSinkServiceProxyTest, TestOnSinksDiscovered) {
  EXPECT_CALL(*mock_service_, Start());
  EXPECT_CALL(mock_sink_discovered_cb_, Run(_)).Times(1);

  proxy_->Start();
  mock_service_->OnSinksDiscovered();
}

TEST_F(DialMediaSinkServiceProxyTest, TestForceSinkDiscoveryCallback) {
  EXPECT_CALL(mock_sink_discovered_cb_, Run(_));

  proxy_->ForceSinkDiscoveryCallback();
  base::RunLoop().RunUntilIdle();
}

TEST_F(DialMediaSinkServiceProxyTest, TestRegisterUnregisterObserver) {
  MockMediaRouter mock_media_router;

  MockMediaSinksObserver observer1(
      &mock_media_router, MediaSource("cast-dial:YouTube"),
      url::Origin::Create(GURL("https://tv.youtube.com")));
  MockMediaSinksObserver observer2(
      &mock_media_router, MediaSource("cast-dial:YouTube"),
      url::Origin::Create(GURL("https://www.youtube.com")));
  MockMediaSinksObserver observer3(
      &mock_media_router, MediaSource("cast-dial:Netflix"),
      url::Origin::Create(GURL("https://www.netflix.com")));
  MockMediaSinksObserver observer4(
      &mock_media_router, MediaSource("cast-dial:Netflix"),
      url::Origin::Create(GURL("https://www.netflix.com")));

  proxy_->RegisterMediaSinksObserver(&observer1);
  proxy_->RegisterMediaSinksObserver(&observer2);
  proxy_->RegisterMediaSinksObserver(&observer3);
  proxy_->RegisterMediaSinksObserver(&observer4);
  base::RunLoop().RunUntilIdle();

  std::vector<MediaSink> sinks{
      MediaSink("sink id 1", "sink name 1", SinkIconType::GENERIC)};

  EXPECT_CALL(observer1, OnSinksReceived(sinks));
  EXPECT_CALL(observer2, OnSinksReceived(sinks));
  mock_service_->OnAvailableSinksUpdated("YouTube", sinks);
  base::RunLoop().RunUntilIdle();

  EXPECT_CALL(observer3, OnSinksReceived(sinks));
  EXPECT_CALL(observer4, OnSinksReceived(sinks));
  mock_service_->OnAvailableSinksUpdated("Netflix", sinks);
  base::RunLoop().RunUntilIdle();

  proxy_->UnregisterMediaSinksObserver(&observer1);
  proxy_->UnregisterMediaSinksObserver(&observer2);
  EXPECT_CALL(observer1, OnSinksReceived(sinks)).Times(0);
  EXPECT_CALL(observer2, OnSinksReceived(sinks)).Times(0);
  mock_service_->OnAvailableSinksUpdated("YouTube", sinks);
  base::RunLoop().RunUntilIdle();
}

}  // namespace media_router
