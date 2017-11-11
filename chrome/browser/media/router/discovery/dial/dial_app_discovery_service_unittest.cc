// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/dial/dial_app_discovery_service.h"

#include "base/strings/stringprintf.h"
#include "base/test/mock_callback.h"
#include "base/test/simple_test_clock.h"
#include "chrome/browser/media/router/discovery/dial/dial_app_info_fetcher.h"
#include "chrome/browser/media/router/discovery/dial/parsed_dial_device_description.h"
#include "chrome/browser/media/router/discovery/dial/safe_dial_app_info_parser.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SaveArg;

namespace {

// Matches ParsedDialAppInfo.
MATCHER_P(AppInfoEquals, expected, "") {
  return expected.app_name == arg.app_name &&
         expected.app_status == arg.app_status;
}

media_router::ParsedDialAppInfo CreateParsedDialAppInfo(int num) {
  return media_router::ParsedDialAppInfo{
      base::StringPrintf("YoutTube%d", num),
      media_router::DialAppStatus::AVAILABLE};
}

chrome::mojom::DialAppInfoPtr CreateDialAppInfoPtr(int num) {
  chrome::mojom::DialAppInfoPtr app_info_ptr =
      chrome::mojom::DialAppInfo::New();
  app_info_ptr->name = base::StringPrintf("YoutTube%d", num);
  app_info_ptr->state = chrome::mojom::DialAppState::RUNNING;

  return app_info_ptr;
}

GURL CreateAppUrl(int num) {
  return GURL(base::StringPrintf("http://127.0.0.1/apps/YouTube%d", num));
}

}  // namespace

namespace media_router {

class TestSafeDialAppInfoParser : public SafeDialAppInfoParser {
 public:
  ~TestSafeDialAppInfoParser() override {}

  MOCK_METHOD2(Start,
               void(const std::string& xml_text,
                    const AppInfoCallback& callback));
};

class DialAppDiscoveryServiceTest : public ::testing::Test {
 public:
  DialAppDiscoveryServiceTest()
      : dial_app_discovery_service_(mock_success_cb_.Get(),
                                    mock_error_cb_.Get()),
        app_url_("http://127.0.0.1/apps/YouTube1") {
    auto clock = base::MakeUnique<base::SimpleTestClock>();
    test_clock_ = clock.get();
    auto parser = base::MakeUnique<TestSafeDialAppInfoParser>();
    test_parser_ = parser.get();
    dial_app_discovery_service_.SetClockForTest(std::move(clock));
    dial_app_discovery_service_.SetParserForTest(std::move(parser));
  }

  void AddToCache(int num) {
    ParsedDialAppInfo parsed_app_info = CreateParsedDialAppInfo(num);
    DialAppDiscoveryService::CacheEntry cache_entry;
    cache_entry.expire_time =
        test_clock_->Now() + base::TimeDelta::FromHours(1);
    cache_entry.app_info = parsed_app_info;

    GURL app_url = CreateAppUrl(num);
    dial_app_discovery_service_.app_info_cache_[app_url] = cache_entry;
  }

 protected:
  base::MockCallback<DialAppDiscoveryService::DialAppInfoParseSuccessCallback>
      mock_success_cb_;
  base::MockCallback<DialAppDiscoveryService::DialAppInfoParseErrorCallback>
      mock_error_cb_;

  base::SimpleTestClock* test_clock_;
  TestSafeDialAppInfoParser* test_parser_;
  DialAppDiscoveryService dial_app_discovery_service_;
  const content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
  const net::TestURLFetcherFactory factory_;
  const GURL app_url_;
};

TEST_F(DialAppDiscoveryServiceTest, TestFechDialAppInfoFromCache) {
  ParsedDialAppInfo parsed_app_info = CreateParsedDialAppInfo(1);
  EXPECT_CALL(mock_success_cb_, Run(app_url_, AppInfoEquals(parsed_app_info)));

  // Adds a cache entry.
  AddToCache(1);
  dial_app_discovery_service_.FetchDialAppInfo(app_url_,
                                               profile_.GetRequestContext());
}

TEST_F(DialAppDiscoveryServiceTest, TestFechDialAppInfoFromCacheExpiredEntry) {
  EXPECT_CALL(mock_success_cb_, Run(_, _)).Times(0);

  // Adds a cache entry.
  AddToCache(1);
  // Cache expires
  test_clock_->Advance(base::TimeDelta::FromHours(2));
  dial_app_discovery_service_.FetchDialAppInfo(app_url_,
                                               profile_.GetRequestContext());
  EXPECT_TRUE(dial_app_discovery_service_.app_info_cache_.empty());
}

TEST_F(DialAppDiscoveryServiceTest, TestFetchDialAppInfoFetchURL) {
  ParsedDialAppInfo parsed_app_info = CreateParsedDialAppInfo(1);
  EXPECT_CALL(mock_success_cb_, Run(app_url_, AppInfoEquals(parsed_app_info)));
  dial_app_discovery_service_.FetchDialAppInfo(app_url_,
                                               profile_.GetRequestContext());

  EXPECT_CALL(*test_parser_, Start(_, _))
      .WillOnce(Invoke(
          [](const std::string& xml_text,
             const TestSafeDialAppInfoParser::AppInfoCallback& callback) {
            chrome::mojom::DialAppInfoPtr mojo_app_info =
                CreateDialAppInfoPtr(1);
            callback.Run(std::move(mojo_app_info),
                         chrome::mojom::DialAppInfoParsingError::kNone);
          }));
  net::TestURLFetcher* test_fetcher =
      factory_.GetFetcherByID(DialAppInfoFetcher::kURLFetcherIDForTest);
  test_fetcher->set_response_code(net::HTTP_OK);
  test_fetcher->SetResponseString("<xml>appInfo</xml>");
  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
}

TEST_F(DialAppDiscoveryServiceTest, TestFetchDialAppInfoFetchURLError) {
  ParsedDialAppInfo parsed_app_info = CreateParsedDialAppInfo(1);
  EXPECT_CALL(mock_error_cb_, Run(app_url_, _));
  dial_app_discovery_service_.FetchDialAppInfo(app_url_,
                                               profile_.GetRequestContext());

  net::TestURLFetcher* test_fetcher =
      factory_.GetFetcherByID(DialAppInfoFetcher::kURLFetcherIDForTest);
  test_fetcher->set_response_code(net::HTTP_NOT_FOUND);
  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
}

TEST_F(DialAppDiscoveryServiceTest, TestFetchDialAppInfoParseError) {
  ParsedDialAppInfo parsed_app_info = CreateParsedDialAppInfo(1);
  EXPECT_CALL(mock_error_cb_,
              Run(app_url_, "Failed to parse app info XML in utility process"));
  dial_app_discovery_service_.FetchDialAppInfo(app_url_,
                                               profile_.GetRequestContext());

  EXPECT_CALL(*test_parser_, Start(_, _))
      .WillOnce(Invoke(
          [](const std::string& xml_text,
             const TestSafeDialAppInfoParser::AppInfoCallback& callback) {
            callback.Run(nullptr,
                         chrome::mojom::DialAppInfoParsingError::kMissingName);
          }));
  net::TestURLFetcher* test_fetcher =
      factory_.GetFetcherByID(DialAppInfoFetcher::kURLFetcherIDForTest);
  test_fetcher->set_response_code(net::HTTP_OK);
  test_fetcher->SetResponseString("<xml>appInfo</xml>");
  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
}

TEST_F(DialAppDiscoveryServiceTest, TestSafeParserProperlyCreated) {
  GURL app_url1 = CreateAppUrl(1);
  GURL app_url2 = CreateAppUrl(2);
  GURL app_url3 = CreateAppUrl(3);

  chrome::mojom::DialAppInfoPtr mojo_app_info1 = CreateDialAppInfoPtr(1);
  chrome::mojom::DialAppInfoPtr mojo_app_info2 = CreateDialAppInfoPtr(2);
  chrome::mojom::DialAppInfoPtr mojo_app_info3 = CreateDialAppInfoPtr(3);

  dial_app_discovery_service_.pending_app_urls_.insert(app_url1);
  dial_app_discovery_service_.pending_app_urls_.insert(app_url2);
  dial_app_discovery_service_.pending_app_urls_.insert(app_url3);

  EXPECT_CALL(*test_parser_, Start(_, _)).Times(3);
  dial_app_discovery_service_.OnDialAppInfoFetchComplete(app_url1, "");
  dial_app_discovery_service_.OnDialAppInfoFetchComplete(app_url2, "");
  dial_app_discovery_service_.OnDialAppInfoFetchComplete(app_url3, "");
  EXPECT_TRUE(dial_app_discovery_service_.parser_);

  EXPECT_CALL(mock_success_cb_, Run(_, _)).Times(3);
  dial_app_discovery_service_.OnParsedDialAppInfo(
      app_url1, std::move(mojo_app_info1),
      chrome::mojom::DialAppInfoParsingError::kNone);
  dial_app_discovery_service_.OnParsedDialAppInfo(
      app_url2, std::move(mojo_app_info2),
      chrome::mojom::DialAppInfoParsingError::kNone);
  EXPECT_TRUE(dial_app_discovery_service_.parser_);

  dial_app_discovery_service_.OnParsedDialAppInfo(
      app_url3, std::move(mojo_app_info3),
      chrome::mojom::DialAppInfoParsingError::kNone);
  EXPECT_FALSE(dial_app_discovery_service_.parser_);
}

TEST_F(DialAppDiscoveryServiceTest,
       TestGetAvailabilityFromAppInfoNetflixAvailable) {
  GURL app_url("http://127.0.0.1/apps/Netflix");
  chrome::mojom::DialAppInfoPtr mojo_app_info =
      chrome::mojom::DialAppInfo::New();
  mojo_app_info->name = "Netflix";
  mojo_app_info->state = chrome::mojom::DialAppState::RUNNING;
  mojo_app_info->capabilities = std::string("websocket");

  ParsedDialAppInfo expected_app_info{"Netflix", DialAppStatus::AVAILABLE};
  EXPECT_CALL(mock_success_cb_, Run(app_url, AppInfoEquals(expected_app_info)));
  dial_app_discovery_service_.OnParsedDialAppInfo(
      app_url, std::move(mojo_app_info),
      chrome::mojom::DialAppInfoParsingError::kNone);
}

TEST_F(DialAppDiscoveryServiceTest,
       TestGetAvailabilityFromAppInfoNetflixUnavailable) {
  GURL app_url("http://127.0.0.1/apps/Netflix");
  chrome::mojom::DialAppInfoPtr mojo_app_info =
      chrome::mojom::DialAppInfo::New();
  mojo_app_info->name = "Netflix";
  mojo_app_info->state = chrome::mojom::DialAppState::RUNNING;
  mojo_app_info->capabilities = std::string("xyz");

  ParsedDialAppInfo expected_app_info{"Netflix", DialAppStatus::UNAVAILABLE};
  EXPECT_CALL(mock_success_cb_, Run(app_url, AppInfoEquals(expected_app_info)));
  dial_app_discovery_service_.OnParsedDialAppInfo(
      app_url, std::move(mojo_app_info),
      chrome::mojom::DialAppInfoParsingError::kNone);
}

}  // namespace media_router
