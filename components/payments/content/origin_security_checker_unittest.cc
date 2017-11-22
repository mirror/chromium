// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/origin_security_checker.h"

#include "base/command_line.h"
#include "content/public/common/content_switches.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace payments {
namespace {

TEST(OriginSecurityCheckerTest, IsOriginSecure) {
  EXPECT_FALSE(OriginSecurityChecker::IsOriginSecure(GURL()));
  EXPECT_FALSE(
      OriginSecurityChecker::IsOriginSecure(GURL("http://example.com")));
  EXPECT_TRUE(OriginSecurityChecker::IsOriginSecure(GURL("http://localhost")));
  EXPECT_TRUE(OriginSecurityChecker::IsOriginSecure(GURL("http://127.0.0.1")));
  EXPECT_TRUE(OriginSecurityChecker::IsOriginSecure(GURL("http://[::1]")));
  EXPECT_TRUE(OriginSecurityChecker::IsOriginSecure(
      GURL("file:///home/user/test.html")));
  EXPECT_TRUE(
      OriginSecurityChecker::IsOriginSecure(GURL("https://example.com")));
}

TEST(OriginSecurityCheckerTest, IsSchemeCryptographic) {
  EXPECT_FALSE(OriginSecurityChecker::IsSchemeCryptographic(GURL()));
  EXPECT_FALSE(
      OriginSecurityChecker::IsSchemeCryptographic(GURL("http://example.com")));
  EXPECT_TRUE(OriginSecurityChecker::IsSchemeCryptographic(
      GURL("https://example.com")));
}

TEST(OriginSecurityCheckerTest, IsOriginLocalhostOrFile) {
  EXPECT_FALSE(OriginSecurityChecker::IsOriginLocalhostOrFile(GURL()));
  EXPECT_FALSE(OriginSecurityChecker::IsOriginLocalhostOrFile(
      GURL("http://example.com")));
  EXPECT_FALSE(OriginSecurityChecker::IsOriginLocalhostOrFile(
      GURL("https://example.com")));
  EXPECT_TRUE(
      OriginSecurityChecker::IsOriginLocalhostOrFile(GURL("http://localhost")));
  EXPECT_TRUE(
      OriginSecurityChecker::IsOriginLocalhostOrFile(GURL("http://127.0.0.1")));
  EXPECT_TRUE(
      OriginSecurityChecker::IsOriginLocalhostOrFile(GURL("http://[::1]")));
  EXPECT_TRUE(OriginSecurityChecker::IsOriginLocalhostOrFile(
      GURL("file:///home/user/test.html")));
}

TEST(OriginSecurityCheckerTest, IsValidWebPaymentsUrl) {
  GURL url;
  std::string error_message_suffix;

  EXPECT_FALSE(OriginSecurityChecker::IsValidWebPaymentsUrl(
      "", &url, &error_message_suffix));

  EXPECT_FALSE(url.is_valid());
  EXPECT_EQ("is an empty string.", error_message_suffix);

  EXPECT_FALSE(OriginSecurityChecker::IsValidWebPaymentsUrl(
      "\xc0\x80", &url, &error_message_suffix));

  EXPECT_FALSE(url.is_valid());
  EXPECT_EQ("is not valid unicode.", error_message_suffix);

  EXPECT_FALSE(OriginSecurityChecker::IsValidWebPaymentsUrl(
      "http://localhost", &url, &error_message_suffix));

  EXPECT_FALSE(url.is_valid());
  std::string expected =
      "is not a URL with HTTPS scheme (or HTTP scheme for "
      "localhost with --allow-web-payments-localhost-urls command line flag).";
  EXPECT_EQ(expected, error_message_suffix);

  EXPECT_FALSE(OriginSecurityChecker::IsValidWebPaymentsUrl(
      "http://example.com", &url, &error_message_suffix));

  EXPECT_FALSE(url.is_valid());
  EXPECT_EQ(expected, error_message_suffix);

  EXPECT_FALSE(OriginSecurityChecker::IsValidWebPaymentsUrl(
      "https://", &url, &error_message_suffix));

  EXPECT_FALSE(url.is_valid());
  EXPECT_EQ("is not a valid URL.", error_message_suffix);

  EXPECT_FALSE(OriginSecurityChecker::IsValidWebPaymentsUrl(
      "https://username@example.com", &url, &error_message_suffix));

  EXPECT_EQ("contains username or password.", error_message_suffix);

  EXPECT_FALSE(OriginSecurityChecker::IsValidWebPaymentsUrl(
      "https://username:password@example.com", &url, &error_message_suffix));

  EXPECT_EQ("contains username or password.", error_message_suffix);

  EXPECT_TRUE(OriginSecurityChecker::IsValidWebPaymentsUrl(
      "https://example.com", &url, &error_message_suffix));

  EXPECT_TRUE(url.is_valid());
  EXPECT_EQ(GURL("https://example.com"), url);

  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kAllowWebPaymentsLocalhostUrls);

  EXPECT_TRUE(OriginSecurityChecker::IsValidWebPaymentsUrl(
      "http://localhost", &url, &error_message_suffix));

  EXPECT_TRUE(url.is_valid());
  EXPECT_EQ(GURL("http://localhost"), url);

  EXPECT_TRUE(OriginSecurityChecker::IsValidWebPaymentsUrl(
      "http://127.0.0.1", &url, &error_message_suffix));

  EXPECT_TRUE(url.is_valid());
  EXPECT_EQ(GURL("http://127.0.0.1"), url);

  EXPECT_TRUE(OriginSecurityChecker::IsValidWebPaymentsUrl(
      "http://[::1]", &url, &error_message_suffix));

  EXPECT_TRUE(url.is_valid());
  EXPECT_EQ(GURL("http://[::1]"), url);

  EXPECT_TRUE(OriginSecurityChecker::IsValidWebPaymentsUrl(
      "https://example.com", &url, &error_message_suffix));

  EXPECT_TRUE(url.is_valid());
  EXPECT_EQ(GURL("https://example.com"), url);

  EXPECT_FALSE(OriginSecurityChecker::IsValidWebPaymentsUrl(
      "http://example.com", &url, &error_message_suffix));

  EXPECT_EQ(expected, error_message_suffix);
}

}  // namespace
}  // namespace payments
