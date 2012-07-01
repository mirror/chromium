// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/csp_validator.h"
#include "testing/gtest/include/gtest/gtest.h"

using extensions::csp_validator::ContentSecurityPolicyIsLegal;
using extensions::csp_validator::ContentSecurityPolicyIsSecure;
using extensions::csp_validator::ContentSecurityPolicyIsSandboxed;
using extensions::Extension;

TEST(ExtensionCSPValidator, IsLegal) {
  EXPECT_TRUE(ContentSecurityPolicyIsLegal("foo"));
  EXPECT_TRUE(ContentSecurityPolicyIsLegal(
      "default-src 'self'; script-src http://www.google.com"));
  EXPECT_FALSE(ContentSecurityPolicyIsLegal(
      "default-src 'self';\nscript-src http://www.google.com"));
  EXPECT_FALSE(ContentSecurityPolicyIsLegal(
      "default-src 'self';\rscript-src http://www.google.com"));
}

TEST(ExtensionCSPValidator, IsSecure) {
  EXPECT_FALSE(ContentSecurityPolicyIsSecure(""));
  EXPECT_FALSE(ContentSecurityPolicyIsSecure("img-src https://google.com"));

  EXPECT_FALSE(ContentSecurityPolicyIsSecure("default-src *"));
  EXPECT_TRUE(ContentSecurityPolicyIsSecure("default-src 'self'"));
  EXPECT_TRUE(ContentSecurityPolicyIsSecure("default-src 'none'"));
  EXPECT_FALSE(ContentSecurityPolicyIsSecure(
      "default-src 'self' ftp://google.com"));
  EXPECT_TRUE(ContentSecurityPolicyIsSecure(
      "default-src 'self' https://google.com"));

  EXPECT_FALSE(ContentSecurityPolicyIsSecure(
      "default-src *; default-src 'self'"));
  EXPECT_TRUE(ContentSecurityPolicyIsSecure(
      "default-src 'self'; default-src *"));
  EXPECT_FALSE(ContentSecurityPolicyIsSecure(
      "default-src 'self'; default-src *; script-src *; script-src 'self'"));
  EXPECT_TRUE(ContentSecurityPolicyIsSecure(
      "default-src 'self'; default-src *; script-src 'self'; script-src *"));

  EXPECT_FALSE(ContentSecurityPolicyIsSecure(
      "default-src *; script-src 'self'"));
  EXPECT_FALSE(ContentSecurityPolicyIsSecure(
      "default-src *; script-src 'self'; img-src 'self'"));
  EXPECT_TRUE(ContentSecurityPolicyIsSecure(
      "default-src *; script-src 'self'; object-src 'self'"));
  EXPECT_TRUE(ContentSecurityPolicyIsSecure(
      "script-src 'self'; object-src 'self'"));

  EXPECT_FALSE(ContentSecurityPolicyIsSecure("default-src 'unsafe-inline'"));
  EXPECT_FALSE(ContentSecurityPolicyIsSecure("default-src 'unsafe-eval'"));
  EXPECT_FALSE(ContentSecurityPolicyIsSecure(
      "default-src 'unsafe-inline' 'none'"));
  EXPECT_FALSE(ContentSecurityPolicyIsSecure(
      "default-src 'self' http://google.com"));
  EXPECT_TRUE(ContentSecurityPolicyIsSecure(
      "default-src 'self' https://google.com"));
  EXPECT_TRUE(ContentSecurityPolicyIsSecure(
      "default-src 'self' chrome://resources"));
  EXPECT_TRUE(ContentSecurityPolicyIsSecure(
      "default-src 'self' chrome-extension://aabbcc"));
  EXPECT_TRUE(ContentSecurityPolicyIsSecure(
      "default-src 'self' chrome-extension-resource://aabbcc"));
  EXPECT_FALSE(ContentSecurityPolicyIsSecure(
      "default-src 'self' https:"));
  EXPECT_FALSE(ContentSecurityPolicyIsSecure(
      "default-src 'self' http:"));
  EXPECT_FALSE(ContentSecurityPolicyIsSecure(
      "default-src 'self' https://*"));
  EXPECT_FALSE(ContentSecurityPolicyIsSecure(
      "default-src 'self' *"));
  EXPECT_FALSE(ContentSecurityPolicyIsSecure(
      "default-src 'self' google.com"));
  EXPECT_TRUE(ContentSecurityPolicyIsSecure(
      "default-src 'self' https://*.google.com"));
}

TEST(ExtensionCSPValidator, IsSandboxed) {
  EXPECT_FALSE(ContentSecurityPolicyIsSandboxed("", Extension::TYPE_EXTENSION));
  EXPECT_FALSE(ContentSecurityPolicyIsSandboxed(
      "img-src https://google.com", Extension::TYPE_EXTENSION));

  // Sandbox directive is required.
  EXPECT_TRUE(ContentSecurityPolicyIsSandboxed(
      "sandbox", Extension::TYPE_EXTENSION));

  // Additional sandbox tokens are OK.
  EXPECT_TRUE(ContentSecurityPolicyIsSandboxed(
      "sandbox allow-scripts", Extension::TYPE_EXTENSION));
  // Except for allow-same-origin.
  EXPECT_FALSE(ContentSecurityPolicyIsSandboxed(
      "sandbox allow-same-origin", Extension::TYPE_EXTENSION));

  // Additional directives are OK.
  EXPECT_TRUE(ContentSecurityPolicyIsSandboxed(
      "sandbox; img-src https://google.com", Extension::TYPE_EXTENSION));

  // Extensions allow navigation and popups, platform apps don't.
  EXPECT_TRUE(ContentSecurityPolicyIsSandboxed(
      "sandbox allow-top-navigation", Extension::TYPE_EXTENSION));
  EXPECT_FALSE(ContentSecurityPolicyIsSandboxed(
      "sandbox allow-top-navigation", Extension::TYPE_PLATFORM_APP));
  EXPECT_TRUE(ContentSecurityPolicyIsSandboxed(
      "sandbox allow-popups", Extension::TYPE_EXTENSION));
  EXPECT_FALSE(ContentSecurityPolicyIsSandboxed(
      "sandbox allow-popups", Extension::TYPE_PLATFORM_APP));
}
