// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "components/signin/core/browser/chrome_connected_header_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr bool kIsHeaderRequest = false;
constexpr char kUrl[] = "https://accounts.google.com/";
constexpr char kAccountId[] = "1234567890";
constexpr int kProfileModeMask = 3;

}  //  namespace

TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest,
     GaiaIdIncludedWithMirrorDisabled) {
  signin::ChromeConnectedHeaderHelper helper(false /* is_mirror_enabled */);
  std::string header =
      helper.BuildRequestHeader(kIsHeaderRequest, GURL(base::StringPiece(kUrl)),
                                kAccountId, kProfileModeMask);

  const std::string kGaiaId = std::string("id=") + kAccountId;
  EXPECT_NE(std::string::npos, header.find(kGaiaId));
}

TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest,
     GaiaIdIncludedWithMirrorEnabled) {
  signin::ChromeConnectedHeaderHelper helper(true /* is_mirror_enabled */);
  std::string header =
      helper.BuildRequestHeader(kIsHeaderRequest, GURL(base::StringPiece(kUrl)),
                                kAccountId, kProfileModeMask);

  const std::string kGaiaId = std::string("id=") + kAccountId;
  EXPECT_NE(std::string::npos, header.find(kGaiaId));
}

TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest, MirrorEnabled) {
  signin::ChromeConnectedHeaderHelper helper(true /* is_mirror_enabled */);
  std::string header =
      helper.BuildRequestHeader(kIsHeaderRequest, GURL(base::StringPiece(kUrl)),
                                kAccountId, kProfileModeMask);

  const std::string kAccountConsistency =
      std::string("enable_account_consistency=true");
  EXPECT_NE(std::string::npos, header.find(kAccountConsistency));
}

TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest, MirrorDisabled) {
  signin::ChromeConnectedHeaderHelper helper(false /* is_mirror_enabled */);
  std::string header =
      helper.BuildRequestHeader(kIsHeaderRequest, GURL(base::StringPiece(kUrl)),
                                kAccountId, kProfileModeMask);

  const std::string kAccountConsistency =
      std::string("enable_account_consistency=false");
  EXPECT_NE(std::string::npos, header.find(kAccountConsistency));
}

TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest,
     ProfileModeMaskWithMirrorEnabled) {
  signin::ChromeConnectedHeaderHelper helper(true /* is_mirror_enabled */);
  std::string header =
      helper.BuildRequestHeader(kIsHeaderRequest, GURL(base::StringPiece(kUrl)),
                                kAccountId, kProfileModeMask);

  const std::string kProfileModeMaskStr =
      std::string("mode=") + std::to_string(kProfileModeMask);
  EXPECT_NE(std::string::npos, header.find(kProfileModeMaskStr));
}

TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest,
     ProfileModeMaskWithMirrorDisabled) {
  signin::ChromeConnectedHeaderHelper helper(false /* is_mirror_enabled */);
  std::string header =
      helper.BuildRequestHeader(kIsHeaderRequest, GURL(base::StringPiece(kUrl)),
                                kAccountId, kProfileModeMask);

  const std::string kProfileModeMaskStr =
      std::string("mode=") + std::to_string(kProfileModeMask);
  EXPECT_NE(std::string::npos, header.find(kProfileModeMaskStr));
}

#if defined(OS_CHROMEOS)
// On ChromeOS, an empty account id means that it is either a guest session or a
// public session. Account consistency must be enforced, if set.
TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest,
     EmptyAccountIdOnChromeOSWithMirrorDisabled) {
  signin::ChromeConnectedHeaderHelper helper(false /* is_mirror_enabled */);
  std::string header =
      helper.BuildRequestHeader(kIsHeaderRequest, GURL(base::StringPiece(kUrl)),
                                "" /* account id */, kProfileModeMask);
  const std::string kAccountConsistencyUnset =
      std::string("enable_account_consistency=false");
  EXPECT_NE(std::string::npos, header.find(kAccountConsistencyUnset));
}

TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest,
     EmptyAccountIdOnChromeOSWithMirrorEnabled) {
  signin::ChromeConnectedHeaderHelper helper(true /* is_mirror_enabled */);
  std::string header =
      helper.BuildRequestHeader(kIsHeaderRequest, GURL(base::StringPiece(kUrl)),
                                "" /* account id */, kProfileModeMask);
  const std::string kAccountConsistencySet =
      std::string("enable_account_consistency=true");
  EXPECT_NE(std::string::npos, header.find(kAccountConsistencySet));
}

#else  // !defined(OS_CHROMEOS)
// Test that an empty account id, for non-ChromeOS devices, produces an empty
// header. This is the case when users are not signed into Chrome, but they must
// be allowed to use Google services anyways.
TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest,
     EmptyAccountIdMirrorEnabledNonChromeOS) {
  signin::ChromeConnectedHeaderHelper helper(true /* is_mirror_enabled */);
  std::string header =
      helper.BuildRequestHeader(kIsHeaderRequest, GURL(base::StringPiece(kUrl)),
                                "" /* account id */, kProfileModeMask);
  EXPECT_TRUE(header.empty());
}

TEST(ChromeConnectedHeaderHelperBuildRequestHeaderTest,
     EmptyAccountIdMirrorDisabledNonChromeOS) {
  signin::ChromeConnectedHeaderHelper helper(false /* is_mirror_enabled */);
  std::string header =
      helper.BuildRequestHeader(kIsHeaderRequest, GURL(base::StringPiece(kUrl)),
                                "" /* account id */, kProfileModeMask);
  EXPECT_TRUE(header.empty());
}

#endif
