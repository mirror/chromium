// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "components/signin/core/browser/chrome_connected_header_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

constexpr char kAccountsUrl[] = "https://accounts.google.com/";
constexpr char kAccountId[] = "1234567890";
constexpr int kProfileModeMask = 3;

}  //  namespace

void VerifyGaiaIdInclusion(const bool kIsMirrorEnabled) {
  constexpr char kDriveUrl[] = "https://drive.google.com/";
  signin::ChromeConnectedHeaderHelper helper(kIsMirrorEnabled);
  std::string accounts_header = helper.BuildRequestHeader(
      true /* is_header_request */, GURL(kAccountsUrl), kAccountId,
      kProfileModeMask);
  std::string drive_header =
      helper.BuildRequestHeader(true /* is_header_request */, GURL(kDriveUrl),
                                kAccountId, kProfileModeMask);
  std::string accounts_cookie = helper.BuildRequestHeader(
      false /* is_header_request */, GURL(kAccountsUrl), kAccountId,
      kProfileModeMask);

  const std::string kGaiaId = std::string("id=") + kAccountId;

  // Gaia id is not included in accounts.google.com request headers.
  EXPECT_EQ(std::string::npos, accounts_header.find(kGaiaId));
  // Gaia id is included in drive.google.com request headers.
  EXPECT_NE(std::string::npos, drive_header.find(kGaiaId));

  EXPECT_NE(std::string::npos, accounts_cookie.find(kGaiaId));
}

TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest,
     GaiaIdIncludedWithMirrorDisabled) {
  VerifyGaiaIdInclusion(false /* kIsMirrorEnabled */);
}

TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest,
     GaiaIdIncludedWithMirrorEnabled) {
  VerifyGaiaIdInclusion(true /* kIsMirrorEnabled */);
}

TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest, MirrorEnabled) {
  signin::ChromeConnectedHeaderHelper helper(true /* is_mirror_enabled */);
  std::string header = helper.BuildRequestHeader(true /* is_header_request */,
                                                 GURL(kAccountsUrl), kAccountId,
                                                 kProfileModeMask);
  std::string cookie = helper.BuildRequestHeader(false /* is_header_request */,
                                                 GURL(kAccountsUrl), kAccountId,
                                                 kProfileModeMask);

  const std::string kAccountConsistency =
      std::string("enable_account_consistency=true");
  EXPECT_NE(std::string::npos, header.find(kAccountConsistency));
  EXPECT_NE(std::string::npos, cookie.find(kAccountConsistency));
}

TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest, MirrorDisabled) {
  signin::ChromeConnectedHeaderHelper helper(false /* is_mirror_enabled */);
  std::string header = helper.BuildRequestHeader(true /* is_header_request */,
                                                 GURL(kAccountsUrl), kAccountId,
                                                 kProfileModeMask);
  std::string cookie = helper.BuildRequestHeader(false /* is_header_request */,
                                                 GURL(kAccountsUrl), kAccountId,
                                                 kProfileModeMask);

  const std::string kAccountConsistency =
      std::string("enable_account_consistency=false");
  EXPECT_NE(std::string::npos, header.find(kAccountConsistency));
  EXPECT_NE(std::string::npos, cookie.find(kAccountConsistency));
}

TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest,
     ProfileModeMaskWithMirrorEnabled) {
  signin::ChromeConnectedHeaderHelper helper(true /* is_mirror_enabled */);
  std::string header = helper.BuildRequestHeader(true /* is_header_request */,
                                                 GURL(kAccountsUrl), kAccountId,
                                                 kProfileModeMask);
  std::string cookie = helper.BuildRequestHeader(false /* is_header_request */,
                                                 GURL(kAccountsUrl), kAccountId,
                                                 kProfileModeMask);

  const std::string kProfileModeMaskStr =
      std::string("mode=") + std::to_string(kProfileModeMask);
  EXPECT_NE(std::string::npos, header.find(kProfileModeMaskStr));
  EXPECT_NE(std::string::npos, cookie.find(kProfileModeMaskStr));
}

TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest,
     ProfileModeMaskWithMirrorDisabled) {
  signin::ChromeConnectedHeaderHelper helper(false /* is_mirror_enabled */);
  std::string header = helper.BuildRequestHeader(true /* is_header_request */,
                                                 GURL(kAccountsUrl), kAccountId,
                                                 kProfileModeMask);
  std::string cookie = helper.BuildRequestHeader(false /* is_header_request */,
                                                 GURL(kAccountsUrl), kAccountId,
                                                 kProfileModeMask);

  const std::string kProfileModeMaskStr =
      std::string("mode=") + std::to_string(kProfileModeMask);
  EXPECT_NE(std::string::npos, header.find(kProfileModeMaskStr));
  EXPECT_NE(std::string::npos, cookie.find(kProfileModeMaskStr));
}

#if defined(OS_CHROMEOS)
// On ChromeOS, an empty account id means that it is either a guest session or a
// public session. Account consistency must be enforced, if set.
TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest,
     EmptyAccountIdOnChromeOSWithMirrorDisabled) {
  signin::ChromeConnectedHeaderHelper helper(false /* is_mirror_enabled */);
  std::string header = helper.BuildRequestHeader(
      true /* is_header_request */, GURL(kAccountsUrl), "" /* account id */,
      kProfileModeMask);
  std::string cookie = helper.BuildRequestHeader(
      false /* is_header_request */, GURL(kAccountsUrl), "" /* account id */,
      kProfileModeMask);

  const std::string kAccountConsistencyUnset =
      std::string("enable_account_consistency=false");
  EXPECT_NE(std::string::npos, header.find(kAccountConsistencyUnset));
  EXPECT_NE(std::string::npos, cookie.find(kAccountConsistencyUnset));
}

TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest,
     EmptyAccountIdOnChromeOSWithMirrorEnabled) {
  signin::ChromeConnectedHeaderHelper helper(true /* is_mirror_enabled */);
  std::string header = helper.BuildRequestHeader(
      true /* is_header_request */, GURL(kAccountsUrl), "" /* account id */,
      kProfileModeMask);
  std::string cookie = helper.BuildRequestHeader(
      false /* is_header_request */, GURL(kAccountsUrl), "" /* account id */,
      kProfileModeMask);

  const std::string kAccountConsistencySet =
      std::string("enable_account_consistency=true");
  EXPECT_NE(std::string::npos, header.find(kAccountConsistencySet));
  EXPECT_NE(std::string::npos, cookie.find(kAccountConsistencySet));
}

#else  // OS_CHROMEOS is not defined below.
// Test that an empty account id, for non-ChromeOS devices, produces an empty
// header. This is the case when users are not signed into Chrome, but they must
// be allowed to use Google services anyways.
TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest,
     EmptyAccountIdMirrorEnabledNonChromeOS) {
  signin::ChromeConnectedHeaderHelper helper(true /* is_mirror_enabled */);
  const std::string header = helper.BuildRequestHeader(
      true /* is_header_request */, GURL(kAccountsUrl), "" /* account id */,
      kProfileModeMask);
  const std::string cookie = helper.BuildRequestHeader(
      false /* is_header_request */, GURL(kAccountsUrl), "" /* account id */,
      kProfileModeMask);

  EXPECT_TRUE(header.empty());
  EXPECT_TRUE(cookie.empty());
}

TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest,
     EmptyAccountIdMirrorDisabledNonChromeOS) {
  signin::ChromeConnectedHeaderHelper helper(false /* is_mirror_enabled */);
  std::string header = helper.BuildRequestHeader(
      true /* is_header_request */, GURL(kAccountsUrl), "" /* account id */,
      kProfileModeMask);
  std::string cookie = helper.BuildRequestHeader(
      false /* is_header_request */, GURL(kAccountsUrl), "" /* account id */,
      kProfileModeMask);

  EXPECT_TRUE(header.empty());
  EXPECT_TRUE(cookie.empty());
}

#endif
