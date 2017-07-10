// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_safe_browsing_whitelist_manager.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using url_matcher::URLMatcherCondition;
using url_matcher::StringPattern;

namespace android_webview {

class AwSafeBrowsingWhitelistManagerTest : public testing::Test {
 protected:
  AwSafeBrowsingWhitelistManagerTest() {}

  void SetUp() override {
    wm_.reset(new AwSafeBrowsingWhitelistManager(
        base::ThreadTaskRunnerHandle::Get(),
        base::ThreadTaskRunnerHandle::Get()));
  }

  void TearDown() override { wm_.reset(); }

  base::MessageLoopForIO loop_;
  std::unique_ptr<AwSafeBrowsingWhitelistManager> wm_;
};

TEST_F(AwSafeBrowsingWhitelistManagerTest, WsSchemeCanBeWhitelisted) {
  std::vector<std::string> whitelist;
  whitelist.push_back("ws://google.com");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("ws://google.com")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("wss://google.com")));
}

// Safebrowsing only works with ws/http/https schemes. Default scheme check
// in URL  matcher works for us.
TEST_F(AwSafeBrowsingWhitelistManagerTest, CustomSchemeCannotBeWhitelisted) {
  std::vector<std::string> whitelist;
  whitelist.push_back("foo://google.com");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://google.com")));
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("https://google.com")));
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("foo://google.com")));
};

TEST_F(AwSafeBrowsingWhitelistManagerTest, UrlsWithNoSchemesWork) {
  std::vector<std::string> whitelist;
  whitelist.push_back("google.com");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://google.com")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://www.google.com")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("https://google.com")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("https://www.google.com")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("ws://google.com")));
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("foo://www.google.com")));

  whitelist = std::vector<std::string>();
  whitelist.push_back(".google.com");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://google.com")));
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://www.google.com")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("https://google.com")));
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("https://www.google.com")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("ws://google.com")));
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("foo://www.google.com")));
};

TEST_F(AwSafeBrowsingWhitelistManagerTest, PortInURLsIsIgnored) {
  std::vector<std::string> whitelist;
  whitelist.push_back("http://.www.google.com");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://.www.google.com:80")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://.www.google.com:123")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://.www.google.com:443")));
}

TEST_F(AwSafeBrowsingWhitelistManagerTest, PortInWhitelistIsIgnored) {
  std::vector<std::string> whitelist;
  whitelist.push_back("http://.www.google.com:123");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://.www.google.com")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://.www.google.com:123")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://.www.google.com:78")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://.www.google.com:443")));
}

TEST_F(AwSafeBrowsingWhitelistManagerTest, PathQueryAndReferenceWorks) {
  std::vector<std::string> whitelist;
  whitelist.push_back("http://google.com");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://google.com/a")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://google.com/a/b")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://google.com?test=1")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://google.com/a#a100")));  // ??
};

TEST_F(AwSafeBrowsingWhitelistManagerTest,
       PathQueryAndReferenceWorksWithLeadingDot) {
  std::vector<std::string> whitelist;
  whitelist.push_back("http://.google.com");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://google.com/")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://google.com/a")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://google.com/a/b")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://google.com?test=1")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://google.com/a#a100")));  // ??
};

TEST_F(AwSafeBrowsingWhitelistManagerTest,
       SubdomainsAreAllowedWhenNoLeadingDots) {
  std::vector<std::string> whitelist;
  whitelist.push_back("http://google.com");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://com/")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://google.com")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://a.google.com/")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://b.a.google.com/")));
};

TEST_F(AwSafeBrowsingWhitelistManagerTest,
       SubdomainsAreNotAllowedWhenLeadingDots) {
  std::vector<std::string> whitelist;
  whitelist.push_back("http://.google.com");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://com/")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://google.com")));
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://a.google.com/")));
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://b.a.google.com/")));
};

TEST_F(AwSafeBrowsingWhitelistManagerTest, WildCardNotAccepted) {
  std::vector<std::string> whitelist;
  whitelist.push_back("http://*");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://a.google.com/")));

  whitelist = std::vector<std::string>();
  whitelist.push_back("http://*");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://a.google.com/")));
};

}  // namespace android_webview
