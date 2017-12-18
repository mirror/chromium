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

// Matches DialAppInfo.
MATCHER_P(AppInfoEquals, expected, "") {
  return expected.app_name == arg.app_name &&
         expected.app_status == arg.app_status;
}

media_router::DialAppInfo CreateDialAppInfo(int num) {
  return media_router::DialAppInfo{base::StringPrintf("YoutTube%d", num),
                                   media_router::DialAppStatus::kAvailable};
}

std::unique_ptr<media_router::ParsedDialAppInfo> CreateParsedDialAppInfoPtr(
    int num) {
  std::unique_ptr<media_router::ParsedDialAppInfo> app_info =
      base::MakeUnique<media_router::ParsedDialAppInfo>();
  app_info->name = base::StringPrintf("YoutTube%d", num);
  app_info->state = media_router::DialAppState::kRunning;

  return app_info;
}

GURL CreateAppUrl(int num) {
  return GURL(base::StringPrintf("http://127.0.0.1/apps/YouTube%d", num));
}

}  // namespace

namespace media_router {

class TestSafeDialAppInfoParser : public SafeDialAppInfoParser {
 public:
  TestSafeDialAppInfoParser() : SafeDialAppInfoParser(nullptr) {}
  ~TestSafeDialAppInfoParser() override {}

  MOCK_METHOD1(Parse, void(const std::string& xml_text));

  void Parse(const std::string& xml_text, ParseCallback callback) override {
    parse_callback_ = std::move(callback);
    Parse(xml_text);
  }

  void InvokeParseCallback(std::unique_ptr<ParsedDialAppInfo> app_info,
                           ParsingError parsing_error) {
    if (!parse_callback_)
      return;
    std::move(parse_callback_).Run(std::move(app_info), parsing_error);
  }

 private:
  ParseCallback parse_callback_;
};

class DialAppDiscoveryServiceTest : public ::testing::Test {
 public:
  DialAppDiscoveryServiceTest()
      : dial_app_discovery_service_(nullptr,
                                    mock_success_cb_.Get(),
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
    DialAppInfo parsed_app_info = CreateDialAppInfo(num);
    GURL app_url = CreateAppUrl(num);
    dial_app_discovery_service_.app_info_cache_[app_url] = parsed_app_info;
  }

  void TearDown() override {
    dial_app_discovery_service_.app_info_fetcher_map_.clear();
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
  DialAppInfo parsed_app_info = CreateDialAppInfo(1);
  EXPECT_CALL(mock_success_cb_, Run(app_url_, AppInfoEquals(parsed_app_info)));

  // Adds a cache entry.
  AddToCache(1);
  dial_app_discovery_service_.FetchDialAppInfo(
      app_url_, profile_.GetRequestContext(), false /* force_fetch */);
}

TEST_F(DialAppDiscoveryServiceTest, TestFechDialAppInfoForceFetchSkipCache) {
  DialAppInfo parsed_app_info = CreateDialAppInfo(1);
  EXPECT_CALL(mock_success_cb_, Run(_, _)).Times(0);

  // Adds a cache entry.
  AddToCache(1);
  dial_app_discovery_service_.FetchDialAppInfo(
      app_url_, profile_.GetRequestContext(), true /* force_fetch */);
}

TEST_F(DialAppDiscoveryServiceTest, TestFetchDialAppInfoFetchURL) {
  DialAppInfo app_info = CreateDialAppInfo(1);
  EXPECT_CALL(mock_success_cb_, Run(app_url_, AppInfoEquals(app_info)));
  dial_app_discovery_service_.FetchDialAppInfo(
      app_url_, profile_.GetRequestContext(), false /* force_fetch */);

  EXPECT_CALL(*test_parser_, Parse(_))
      .WillOnce(Invoke([&](const std::string& xml_text) {
        std::unique_ptr<ParsedDialAppInfo> parsed_app_info =
            CreateParsedDialAppInfoPtr(1);
        test_parser_->InvokeParseCallback(
            std::move(parsed_app_info),
            SafeDialAppInfoParser::ParsingError::kNone);
      }));
  net::TestURLFetcher* test_fetcher =
      factory_.GetFetcherByID(DialAppInfoFetcher::kURLFetcherIDForTest);
  test_fetcher->set_response_code(net::HTTP_OK);
  test_fetcher->SetResponseString("<xml>appInfo</xml>");
  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
}

TEST_F(DialAppDiscoveryServiceTest,
       TestFetchDialAppInfoFetchURLTransientError) {
  DialAppInfo parsed_app_info = CreateDialAppInfo(1);
  EXPECT_CALL(mock_error_cb_, Run(app_url_, _)).Times(0);
  dial_app_discovery_service_.FetchDialAppInfo(
      app_url_, profile_.GetRequestContext(), false /* force_fetch */);

  net::TestURLFetcher* test_fetcher =
      factory_.GetFetcherByID(DialAppInfoFetcher::kURLFetcherIDForTest);
  test_fetcher->set_response_code(net::HTTP_TEMPORARY_REDIRECT);
  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
}

TEST_F(DialAppDiscoveryServiceTest, TestFetchDialAppInfoFetchURLError) {
  DialAppInfo parsed_app_info = CreateDialAppInfo(1);
  EXPECT_CALL(mock_error_cb_, Run(app_url_, _));
  dial_app_discovery_service_.FetchDialAppInfo(
      app_url_, profile_.GetRequestContext(), false /* force_fetch */);

  net::TestURLFetcher* test_fetcher =
      factory_.GetFetcherByID(DialAppInfoFetcher::kURLFetcherIDForTest);
  test_fetcher->set_response_code(net::HTTP_NOT_FOUND);
  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);

  EXPECT_CALL(mock_error_cb_, Run(_, _)).Times(0);
  dial_app_discovery_service_.FetchDialAppInfo(
      app_url_, profile_.GetRequestContext(), false /* force_fetch */);
}

TEST_F(DialAppDiscoveryServiceTest, TestFetchDialAppInfoParseError) {
  DialAppInfo parsed_app_info = CreateDialAppInfo(1);
  EXPECT_CALL(mock_error_cb_,
              Run(app_url_, "Failed to parse app info XML in utility process"));
  dial_app_discovery_service_.FetchDialAppInfo(
      app_url_, profile_.GetRequestContext(), false /* force_fetch */);

  EXPECT_CALL(*test_parser_, Parse(_))
      .WillOnce(Invoke([&](const std::string& xml_text) {
        test_parser_->InvokeParseCallback(
            nullptr, SafeDialAppInfoParser::ParsingError::kMissingName);
      }));
  net::TestURLFetcher* test_fetcher =
      factory_.GetFetcherByID(DialAppInfoFetcher::kURLFetcherIDForTest);
  test_fetcher->set_response_code(net::HTTP_OK);
  test_fetcher->SetResponseString("<xml>appInfo</xml>");
  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);

  EXPECT_CALL(mock_error_cb_, Run(app_url_,
                                  "Last fetch failed due to parsing error or "
                                  "unavailable HTTP response."));
  dial_app_discovery_service_.FetchDialAppInfo(
      app_url_, profile_.GetRequestContext(), false /* force_fetch */);
}

TEST_F(DialAppDiscoveryServiceTest, TestGetAvailabilityFromAppInfoAvailable) {
  GURL app_url("http://127.0.0.1/apps/Netflix");
  std::unique_ptr<ParsedDialAppInfo> parsed_app_info =
      base::MakeUnique<ParsedDialAppInfo>();
  parsed_app_info->name = "Netflix";
  parsed_app_info->state = DialAppState::kRunning;

  DialAppInfo expected_app_info{"Netflix", DialAppStatus::kAvailable};
  DialAppDiscoveryService::FetcherStatus out_fetcher_status;
  EXPECT_CALL(mock_success_cb_, Run(app_url, AppInfoEquals(expected_app_info)));
  dial_app_discovery_service_.OnDialAppInfo(
      app_url, &out_fetcher_status, std::move(parsed_app_info),
      SafeDialAppInfoParser::ParsingError::kNone);
}

TEST_F(DialAppDiscoveryServiceTest, TestGetAvailabilityFromAppInfoUnavailable) {
  GURL app_url("http://127.0.0.1/apps/Netflix");
  std::unique_ptr<ParsedDialAppInfo> parsed_app_info =
      base::MakeUnique<ParsedDialAppInfo>();
  parsed_app_info->name = "Netflix";
  parsed_app_info->state = DialAppState::kUnknown;

  DialAppInfo expected_app_info{"Netflix", DialAppStatus::kUnavailable};
  DialAppDiscoveryService::FetcherStatus out_fetcher_status;
  EXPECT_CALL(mock_success_cb_, Run(app_url, AppInfoEquals(expected_app_info)));
  dial_app_discovery_service_.OnDialAppInfo(
      app_url, &out_fetcher_status, std::move(parsed_app_info),
      SafeDialAppInfoParser::ParsingError::kNone);
}

}  // namespace media_router
