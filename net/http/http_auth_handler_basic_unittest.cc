// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_auth_handler_basic.h"

#include <memory>
#include <string>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "net/base/net_errors.h"
#include "net/base/test_completion_callback.h"
#include "net/http/http_auth_challenge_tokenizer.h"
#include "net/http/http_request_info.h"
#include "net/ssl/ssl_info.h"
#include "net/test/gtest_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using net::test::IsOk;

namespace net {

TEST(HttpAuthHandlerBasicTest, GenerateAuthToken) {
  static const struct {
    const char* username;
    const char* password;
    const char* expected_credentials;
  } tests[] = {
    { "foo", "bar", "Basic Zm9vOmJhcg==" },
    // Empty username
    { "", "foobar", "Basic OmZvb2Jhcg==" },
    // Empty password
    { "anon", "", "Basic YW5vbjo=" },
    // Empty username and empty password.
    { "", "", "Basic Og==" },
  };
  GURL origin("http://www.example.com");
  HttpAuthHandlerBasic::Factory factory;
  for (size_t i = 0; i < arraysize(tests); ++i) {
    std::string challenge = "Basic realm=\"Atlantis\"";
    std::unique_ptr<HttpAuthHandler> basic =
        factory.CreateAuthHandlerForScheme("basic");
    ASSERT_TRUE(basic);
    HttpAuthChallengeTokenizer tokenizer(challenge.begin(), challenge.end());
    EXPECT_EQ(OK, basic->HandleInitialChallenge(
                      tokenizer, HttpAuth::AUTH_SERVER, origin, BoundNetLog()));
    AuthCredentials credentials(base::ASCIIToUTF16(tests[i].username),
                                base::ASCIIToUTF16(tests[i].password));
    HttpRequestInfo request_info;
    std::string auth_token;
    TestCompletionCallback callback;
    int rv = basic->GenerateAuthToken(&credentials, request_info,
                                      callback.callback(), &auth_token);
    EXPECT_THAT(rv, IsOk());
    EXPECT_STREQ(tests[i].expected_credentials, auth_token.c_str());
  }
}

TEST(HttpAuthHandlerBasicTest, HandleAnotherChallenge) {
  static const struct {
    const char* challenge;
    HttpAuth::AuthorizationResult expected_rv;
  } tests[] = {
    // The handler is initialized using this challenge.  The first
    // time HandleAnotherChallenge is called with it should cause it
    // to treat the second challenge as a rejection since it is for
    // the same realm.
    {
      "Basic realm=\"First\"",
      HttpAuth::AUTHORIZATION_RESULT_REJECT
    },

    // A challenge for a different realm.
    {
      "Basic realm=\"Second\"",
      HttpAuth::AUTHORIZATION_RESULT_DIFFERENT_REALM
    },

    // Although RFC 2617 isn't explicit about this case, if there is
    // more than one realm directive, we pick the last one.  So this
    // challenge should be treated as being for "First" realm.
    {
      "Basic realm=\"Second\",realm=\"First\"",
      HttpAuth::AUTHORIZATION_RESULT_REJECT
    },

    // And this one should be treated as if it was for "Second."
    {
      "basic realm=\"First\",realm=\"Second\"",
      HttpAuth::AUTHORIZATION_RESULT_DIFFERENT_REALM
    }
  };

  GURL origin("http://www.example.com");
  HttpAuthHandlerBasic::Factory factory;
  std::unique_ptr<HttpAuthHandler> basic =
      factory.CreateAuthHandlerForScheme("basic");
  std::string initial_challenge(tests[0].challenge);
  HttpAuthChallengeTokenizer tokenizer(initial_challenge.begin(),
                                       initial_challenge.end());
  EXPECT_EQ(OK, basic->HandleInitialChallenge(tokenizer, HttpAuth::AUTH_SERVER,
                                              origin, BoundNetLog()));

  for (size_t i = 0; i < arraysize(tests); ++i) {
    std::string challenge(tests[i].challenge);
    HttpAuthChallengeTokenizer tok(challenge.begin(),
                                   challenge.end());
    EXPECT_EQ(tests[i].expected_rv, basic->HandleAnotherChallenge(tok));
  }
}

TEST(HttpAuthHandlerBasicTest, HandleInitialChallenge) {
  static const struct {
    const char* challenge;
    int expected_rv;
    const char* expected_realm;
  } tests[] = {
    // No realm (we allow this even though realm is supposed to be required
    // according to RFC 2617.)
    {
      "Basic",
      OK,
      "",
    },

    // Realm is empty string.
    {
      "Basic realm=\"\"",
      OK,
      "",
    },

    // Realm is valid.
    {
      "Basic realm=\"test_realm\"",
      OK,
      "test_realm",
    },

    // The parser ignores tokens which aren't known.
    {
      "Basic realm=\"test_realm\",unknown_token=foobar",
      OK,
      "test_realm",
    },

    // The parser skips over tokens which aren't known.
    {
      "Basic unknown_token=foobar,realm=\"test_realm\"",
      OK,
      "test_realm",
    },

#if 0
    // TODO(cbentzel): It's unclear what the parser should do in these cases.
    //                 It seems like this should either be treated as invalid,
    //                 or the spaces should be used as a separator.
    {
      "Basic realm=\"test_realm\" unknown_token=foobar",
      OK,
      "test_realm",
    },

    // The parser skips over tokens which aren't known.
    {
      "Basic unknown_token=foobar realm=\"test_realm\"",
      OK,
      "test_realm",
    },
#endif

    // Although RFC 2617 isn't explicit about this case, if there is
    // more than one realm directive, we pick the last one.
    {
      "Basic realm=\"foo\",realm=\"bar\"",
      OK,
      "bar",
    },

    // Handle ISO-8859-1 character as part of the realm. The realm is converted
    // to UTF-8.
    {
      "Basic realm=\"foo-\xE5\"",
      OK,
      "foo-\xC3\xA5",
    },
  };
  HttpAuthHandlerBasic::Factory factory;
  GURL origin("http://www.example.com");
  for (size_t i = 0; i < arraysize(tests); ++i) {
    std::string challenge = tests[i].challenge;
    std::unique_ptr<HttpAuthHandler> basic =
        factory.CreateAuthHandlerForScheme("basic");
    HttpAuthChallengeTokenizer tokenizer(challenge.begin(), challenge.end());
    int rv = basic->HandleInitialChallenge(tokenizer, HttpAuth::AUTH_SERVER,
                                           origin, BoundNetLog());
    EXPECT_EQ(tests[i].expected_rv, rv);
    if (rv == OK)
      EXPECT_EQ(tests[i].expected_realm, basic->realm());
  }
}

TEST(HttpAuthHandlerBasicTest, CreateAuthHandlerForScheme) {
  HttpAuthHandlerBasic::Factory basic_factory;

  EXPECT_TRUE(basic_factory.CreateAuthHandlerForScheme("basic"));
  EXPECT_FALSE(basic_factory.CreateAuthHandlerForScheme("bogus"));
}

TEST(HttpAuthHandlerBasicTest, CreateAndInitPreemptiveAuthHandler_Valid) {
  HttpAuthHandlerBasic::Factory digest_factory;
  HttpAuthCache auth_cache;
  std::string challenge("basic realm=\"Foo\"");
  HttpAuthChallengeTokenizer tokenizer(challenge.begin(), challenge.end());

  HttpAuthCache::Entry* entry =
      auth_cache.Add(GURL("http://example.com/foo").GetOrigin(), "foo", "basic",
                     challenge, AuthCredentials(), "/foo");
  EXPECT_TRUE(digest_factory.CreateAndInitPreemptiveAuthHandler(
      entry, tokenizer, HttpAuth::AUTH_SERVER, BoundNetLog()));
}

TEST(HttpAuthHandlerBasicTest, CreateAndInitPreemptiveAuthHandler_Invalid) {
  HttpAuthHandlerBasic::Factory digest_factory;
  HttpAuthCache auth_cache;
  std::string challenge("digest realm=\"foo\"");
  HttpAuthChallengeTokenizer tokenizer(challenge.begin(), challenge.end());

  HttpAuthCache::Entry* entry =
      auth_cache.Add(GURL("http://example.com").GetOrigin(), "bar", "digest",
                     challenge, AuthCredentials(), "/bar");
  EXPECT_FALSE(digest_factory.CreateAndInitPreemptiveAuthHandler(
      entry, tokenizer, HttpAuth::AUTH_SERVER, BoundNetLog()));
}

}  // namespace net
