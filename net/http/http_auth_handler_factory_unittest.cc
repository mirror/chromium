// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_auth_handler_factory.h"

#include <memory>

#include "base/strings/string_util.h"
#include "net/base/net_errors.h"
#include "net/base/test_completion_callback.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_auth_challenge_tokenizer.h"
#include "net/http/http_auth_handler.h"
#include "net/http/http_auth_scheme.h"
#include "net/http/mock_allow_http_auth_preferences.h"
#include "net/http/url_security_manager.h"
#include "net/ssl/ssl_info.h"
#include "net/test/gtest_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using net::test::IsError;
using net::test::IsOk;

namespace net {

namespace {

class MockHttpAuthHandlerFactory : public HttpAuthHandlerFactory {
 public:
  explicit MockHttpAuthHandlerFactory(const char* scheme)
      : scheme_(base::ToLowerASCII(scheme)) {}
  ~MockHttpAuthHandlerFactory() override {}

  std::unique_ptr<HttpAuthHandler> CreateAuthHandlerForScheme(
      const std::string& scheme) override {
    if (scheme_.empty())
      return scoped_ptr<HttpAuthHandler>();
    EXPECT_EQ(scheme, scheme_) << scheme << " vs. " << scheme_;
    return make_scoped_ptr(new HttpAuthHandlerMock());
  }
  std::unique_ptr<HttpAuthHandler> CreateAndInitPreemptiveAuthHandler(
      HttpAuthCache::Entry* cache_entry,
      HttpAuth::Target target,
      const BoundNetLog& net_log) override {
    EXPECT_EQ(cache_entry->scheme(), scheme_) << cache_entry->scheme()
                                              << " vs. " << scheme_;
    return make_scoped_ptr(new HttpAuthHandlerMock());
  }

 private:
  std::string scheme_;
};

}  // namespace

TEST(HttpAuthHandlerFactoryTest, RegistryFactory) {
  SSLInfo null_ssl_info;
  HttpAuthHandlerRegistryFactory registry_factory;
  GURL gurl("www.google.com");

  std::unique_ptr<HttpAuthHandler> handler;

  // No schemes should be supported in the beginning.
  handler = registry_factory.CreateAuthHandlerForScheme("basic");
  EXPECT_FALSE(handler);

  // Test what happens with a single scheme.
  registry_factory.RegisterSchemeFactory(
      "basic", new MockHttpAuthHandlerFactory("basic"));
  handler = registry_factory.CreateAuthHandlerForScheme("basic");
  EXPECT_TRUE(handler);
  handler = registry_factory.CreateAuthHandlerForScheme("digest");
  EXPECT_FALSE(handler);

  // Test multiple schemes
  registry_factory.RegisterSchemeFactory(
      "digest", new MockHttpAuthHandlerFactory("digest"));
  handler = registry_factory.CreateAuthHandlerForScheme("basic");
  EXPECT_TRUE(handler);
  handler = registry_factory.CreateAuthHandlerForScheme("digest");
  EXPECT_TRUE(handler);
  handler = registry_factory.CreateAuthHandlerForScheme("bogus");
  EXPECT_FALSE(handler);

  // Test replacement of existing schemes.
  registry_factory.RegisterSchemeFactory("digest",
                                         new MockHttpAuthHandlerFactory(""));
  handler = registry_factory.CreateAuthHandlerForScheme("digest");
  EXPECT_FALSE(handler);
  registry_factory.RegisterSchemeFactory(
      "digest", new MockHttpAuthHandlerFactory("digest"));
  handler = registry_factory.CreateAuthHandlerForScheme("digest");
  EXPECT_TRUE(handler);
}

TEST(HttpAuthHandlerFactoryTest, DefaultFactory) {
  std::unique_ptr<HostResolver> host_resolver(new MockHostResolver());
  MockAllowHttpAuthPreferences http_auth_preferences;
  std::unique_ptr<HttpAuthHandlerRegistryFactory> http_auth_handler_factory(
      HttpAuthHandlerFactory::CreateDefault(host_resolver.get()));
  http_auth_handler_factory->SetHttpAuthPreferences(kNegotiateAuthScheme,
                                                    &http_auth_preferences);
  GURL server_origin("http://www.example.com");
  GURL proxy_origin("http://cache.example.com:3128");
  SSLInfo null_ssl_info;
  {
    std::string challenge = "Basic realm=\"FooBar\"";
    HttpAuthChallengeTokenizer tokenizer(challenge.begin(), challenge.end());
    std::unique_ptr<HttpAuthHandler> handler =
        http_auth_handler_factory->CreateAuthHandlerForScheme(
            tokenizer.NormalizedScheme());
    ASSERT_TRUE(handler);
    int rv = handler->HandleInitialChallenge(
        tokenizer, response_info, HttpAuth::AUTH_SERVER, server_origin,
        BoundNetLog(), callback.callback());
    EXPECT_THAT(callback.GetResult(rv), IsOk());
    EXPECT_EQ("basic", handler->auth_scheme());
    EXPECT_STREQ("FooBar", handler->realm().c_str());
    EXPECT_EQ(HttpAuth::AUTH_SERVER, handler->target());
  }
  {
    std::unique_ptr<HttpAuthHandler> handler =
        http_auth_handler_factory->CreateAuthHandlerForScheme("UNSUPPORTED");
    EXPECT_FALSE(handler);
  }
  {
    std::string challenge = "Digest realm=\"FooBar\", nonce=\"xyz\"";
    HttpAuthChallengeTokenizer tokenizer(challenge.begin(), challenge.end());
    std::unique_ptr<HttpAuthHandler> handler =
        http_auth_handler_factory->CreateAuthHandlerForScheme(
            tokenizer.NormalizedScheme());
    ASSERT_TRUE(handler);
    int rv = handler->HandleInitialChallenge(
        tokenizer, response_info, HttpAuth::AUTH_PROXY, proxy_origin,
        BoundNetLog(), callback.callback());
    EXPECT_EQ(callback.GetResult(rv), IsOk());
    ASSERT_FALSE(handler.get() == NULL);
    EXPECT_EQ("digest", handler->auth_scheme());
    EXPECT_STREQ("FooBar", handler->realm().c_str());
    EXPECT_EQ(HttpAuth::AUTH_PROXY, handler->target());
  }
  {
    std::string challenge = "NTLM";
    HttpAuthChallengeTokenizer tokenizer(challenge.begin(), challenge.end());
    std::unique_ptr<HttpAuthHandler> handler =
        http_auth_handler_factory->CreateAuthHandlerForScheme(
            tokenizer.NormalizedScheme());
    ASSERT_TRUE(handler);
    int rv = handler->HandleInitialChallenge(
        tokenizer, response_info, HttpAuth::AUTH_SERVER, server_origin,
        BoundNetLog(), callback.callback());
    EXPECT_EQ(callback.GetResult(rv), IsOk());
    ASSERT_FALSE(handler.get() == NULL);
    EXPECT_EQ("ntlm", handler->auth_scheme());
    EXPECT_STREQ("", handler->realm().c_str());
    EXPECT_EQ(HttpAuth::AUTH_SERVER, handler->target());
  }
  {
    std::string challenge = "Negotiate";
    HttpAuthChallengeTokenizer tokenizer(challenge.begin(), challenge.end());
    std::unique_ptr<HttpAuthHandler> handler =
        http_auth_handler_factory->CreateAuthHandlerForScheme(
            tokenizer.NormalizedScheme());
    ASSERT_TRUE(handler);
    int rv = handler->HandleInitialChallenge(
        tokenizer, response_info, HttpAuth::AUTH_SERVER, server_origin,
        BoundNetLog(), callback.callback());
    rv = callback.GetResult(rv);
// Note the default factory doesn't support Kerberos on Android
#if defined(USE_KERBEROS) && !defined(OS_ANDROID)
    EXPECT_THAT(rv, IsOk());
    ASSERT_FALSE(handler.get() == NULL);
    EXPECT_EQ("negotiate", handler->auth_scheme());
    EXPECT_STREQ("", handler->realm().c_str());
    EXPECT_EQ(HttpAuth::AUTH_SERVER, handler->target());
#else
    EXPECT_THAT(rv, IsError(ERR_UNSUPPORTED_AUTH_SCHEME));
    EXPECT_TRUE(handler.get() == NULL);
#endif  // defined(USE_KERBEROS) && !defined(OS_ANDROID)
  }
}

}  // namespace net
