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
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://google.com/a#a100")));
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
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://google.com/a#a100")));
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

TEST_F(AwSafeBrowsingWhitelistManagerTest,
       MatchSubdomainsInMultipleWhiteLists) {
  std::vector<std::string> whitelist;
  whitelist.push_back("http://a.google.com");
  whitelist.push_back("http://.google.com");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://a.google.com/")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://b.a.google.com/")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://google.com/")));
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://b.google.com/")));
};

TEST_F(AwSafeBrowsingWhitelistManagerTest, VerifyTLDsAreNotSpecial) {
  std::vector<std::string> whitelist;
  whitelist.push_back("http://.com");
  whitelist.push_back("http://co");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://a.google.com/")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://b.a.google.co/")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://com/")));
};

TEST_F(AwSafeBrowsingWhitelistManagerTest, VerifyRandomWildcardsAreIgnored) {
  std::vector<std::string> whitelist;
  whitelist.push_back("http://*.com");
  whitelist.push_back("http://*co");
  whitelist.push_back("http://b.a.*.co");
  whitelist.push_back("http://b.*.*.co");
  whitelist.push_back("http://b.*");
  whitelist.push_back("http://c*");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://a.google.com/")));
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://b.a.google.co/")));
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://com/")));
};

TEST_F(AwSafeBrowsingWhitelistManagerTest, VerifyPrefixOrSuffixOfDomains) {
  std::vector<std::string> whitelist;
  whitelist.push_back("http://google.com");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://ogle.com/")));
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://agoogle.com/")));
};

TEST_F(AwSafeBrowsingWhitelistManagerTest, VerifyIPV4CanBeWhitelisted) {
  std::vector<std::string> whitelist;
  whitelist.push_back("http://google.com");
  whitelist.push_back("http://192.168.1.1");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://google.com/")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://192.168.1.1/")));
};

TEST_F(AwSafeBrowsingWhitelistManagerTest, VerifyIPV4IsNotSegmented) {
  std::vector<std::string> whitelist;
  whitelist.push_back("http://192.168.1.1");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://192.168.1.1/")));
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://1.192.168.1.1/")));
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://192.168.1.0/")));
};

TEST_F(AwSafeBrowsingWhitelistManagerTest, VerifyLeadingDotInIPV4IsNotValid) {
  std::vector<std::string> whitelist;
  whitelist.push_back("http://.192.168.1.1");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://192.168.1.1/")));
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://1.192.168.1.1/")));
};

TEST_F(AwSafeBrowsingWhitelistManagerTest, VerifyMultipleIPV4Works) {
  std::vector<std::string> whitelist;
  whitelist.push_back("http://192.168.1.1");
  whitelist.push_back("http://192.168.1.2");
  whitelist.push_back("http://194.168.1.1");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://192.168.1.1/")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://192.168.1.2/")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://194.168.1.1/")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("https://194.168.1.1/")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://194.168.1.1:443/")));
};

TEST_F(AwSafeBrowsingWhitelistManagerTest, VerifyIPV6CanBeWhiteListed) {
  std::vector<std::string> whitelist;
  whitelist.push_back("http://[20:1:2:3:4:5:]");
  whitelist.push_back("http://[60:a:b:c:d:e:]");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://[20:1:2:3:4:5:]")));
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://[60:a:b:c:d:e:]")));
};

TEST_F(AwSafeBrowsingWhitelistManagerTest,
       VerifyIPV6CannotBeWhiteListedIfBroken) {
  std::vector<std::string> whitelist;
  whitelist.push_back("http://[20:1:2:3:4:5]");
  whitelist.push_back("http://[60:a:b:c:d:e]");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://[20:1:2:3:4:5]")));
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://[60:a:b:c:d:e]")));
};

TEST_F(AwSafeBrowsingWhitelistManagerTest,
       VerifyIPV6WithZerosCanBeWhiteListed) {
  std::vector<std::string> whitelist;
  whitelist.push_back("http://[20:0:0:0:0:0:0:0]");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://[20:0:0:0:0:0:0:0]")));
};

TEST_F(AwSafeBrowsingWhitelistManagerTest, VerifyCapitalizationDoesNotMatter) {
  std::vector<std::string> whitelist;
  whitelist.push_back("http://A.goOGle.Com");
  whitelist.push_back("http://.GOOGLE.COM");
  wm_->SetWhitelistOnUiThread(std::move(whitelist));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://a.google.com/")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://b.a.google.com/")));
  EXPECT_TRUE(wm_->IsURLWhitelisted(GURL("http://google.com/")));
  EXPECT_FALSE(wm_->IsURLWhitelisted(GURL("http://b.google.com/")));
};

}  // namespace android_webview
