// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/url_security_manager.h"

#include <utility>

#include "net/base/net_errors.h"
#include "net/http/http_auth_filter.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace net {

namespace {

struct TestData {
  const char* const url;
  bool succeeds_in_whitelist;
};

const char kTestAuthWhitelist[] = "*example.com,*foobar.com,baz";

// Under Windows the following will be allowed by default:
//    localhost
//    host names without a period.
// In Posix systems (or on Windows if a whitelist is specified explicitly),
// everything depends on the whitelist.
const TestData kTestDataList[] = {
    {"http://localhost", false},
    {"http://bat", false},
    {"http://www.example.com", true},
    {"http://example.com", true},
    {"http://foobar.com", true},
    {"http://boo.foobar.com", true},
    {"http://baz", true},
    {"http://www.exampl.com", false},
    {"http://example.org", false},
    {"http://foobar.net", false},
    {"http://boo.fubar.com", false},
};

}  // namespace

TEST(URLSecurityManager, AmbientCredentialsWhitelist) {
  std::unique_ptr<HttpAuthFilter> auth_filter(
      new HttpAuthFilterWhitelist(kTestAuthWhitelist));
  std::unique_ptr<URLSecurityManager> url_security_manager(
      URLSecurityManager::Create(
          auth_filter.Pass(), std::unique_ptr<HttpAuthFilter>(),
          URLSecurityManager::ALLOW_AMBIENT_CREDENTIALS_WITH_NTLM));

  auth_filter.reset(new HttpAuthFilterWhitelist(kTestAuthWhitelist));
  std::unique_ptr<URLSecurityManager> url_security_manager_no_ntlm(
      URLSecurityManager::Create(auth_filter.Pass(),
                                 std::unique_ptr<HttpAuthFilter>(),
                                 URLSecurityManager::DISALLOW_NTLM));

  for (size_t i = 0; i < arraysize(kTestDataList); ++i) {
    GURL gurl(kTestDataList[i].url);
    SCOPED_TRACE(testing::Message() << "Run: " << i << " URL: '" << gurl
                                    << "'");

    EXPECT_EQ(kTestDataList[i].succeeds_in_whitelist,
              url_security_manager->CanUseAmbientCredentialsForNegotiate(gurl));
    EXPECT_EQ(kTestDataList[i].succeeds_in_whitelist,
              url_security_manager->CanUseAmbientCredentialsForNTLM(gurl));
    EXPECT_FALSE(
        url_security_manager_no_ntlm->CanUseAmbientCredentialsForNTLM(gurl));
  }
}

TEST(URLSecurityManager, AuthDelegateWhitelist) {
  std::unique_ptr<HttpAuthFilter> auth_filter(
      new HttpAuthFilterWhitelist(kTestAuthWhitelist));
  std::unique_ptr<URLSecurityManager> url_security_manager(
      URLSecurityManager::Create(
          std::unique_ptr<HttpAuthFilter>(), auth_filter.Pass(),
          URLSecurityManager::ALLOW_AMBIENT_CREDENTIALS_WITH_NTLM));
  ASSERT_TRUE(url_security_manager.get());

  for (size_t i = 0; i < arraysize(kTestDataList); ++i) {
    GURL gurl(kTestDataList[i].url);
    EXPECT_EQ(kTestDataList[i].succeeds_in_whitelist,
              url_security_manager->CanDelegate(gurl))
        << " Run: " << i << " URL: '" << gurl << "'";
  }
}

#if defined(OS_POSIX)

TEST(URLSecurityManager, EmptyWhitelistst_Posix) {
  std::unique_ptr<URLSecurityManager> url_security_manager(
      URLSecurityManager::Create());
  ASSERT_TRUE(url_security_manager.get());

  for (size_t i = 0; i < arraysize(kTestDataList); ++i) {
    GURL gurl(kTestDataList[i].url);
    EXPECT_FALSE(url_security_manager->CanDelegate(gurl));
    EXPECT_FALSE(url_security_manager->CanUseAmbientCredentialsForNTLM(gurl));
    EXPECT_FALSE(
        url_security_manager->CanUseAmbientCredentialsForNegotiate(gurl));
  }
}

#endif  // OS_POSIX

}  // namespace net
