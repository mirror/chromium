// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/signin_promo.h"

#include "chrome/browser/signin/signin_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace signin {

class SigninPromoTest : public ::testing::Test {};

TEST_F(SigninPromoTest, TestPromoURL) {
  GURL expected_url_1(
      "chrome://chrome-signin/"
      "?access_point=0&reason=0&auto_close=1&constrained=1");
  EXPECT_EQ(expected_url_1,
            GetPromoURLForDialog(
                signin_metrics::AccessPoint::ACCESS_POINT_START_PAGE,
                signin_metrics::Reason::REASON_SIGNIN_PRIMARY_ACCOUNT, true));
  GURL expected_url_2(
      "chrome://chrome-signin/?access_point=15&reason=3&constrained=1");
  EXPECT_EQ(expected_url_2,
            GetPromoURLForDialog(
                signin_metrics::AccessPoint::ACCESS_POINT_SIGNIN_PROMO,
                signin_metrics::Reason::REASON_UNLOCK, false));
}

TEST_F(SigninPromoTest, TestURLWithForceSignin) {
  signin_util::SetForceSigninForTesting(true);
  GURL expected_url_1(
      "chrome://chrome-signin/"
      "?access_point=0&reason=0&auto_close=1&constrained=1&flow=enterprisefsi");
  EXPECT_EQ(expected_url_1,
            GetPromoURLForDialog(
                signin_metrics::AccessPoint::ACCESS_POINT_START_PAGE,
                signin_metrics::Reason::REASON_SIGNIN_PRIMARY_ACCOUNT, true));
  signin_util::ResetForceSigninForTesting();
}

TEST_F(SigninPromoTest, TestReauthURL) {
  GURL expected_url_1(
      "chrome://chrome-signin/"
      "?access_point=0&reason=0&auto_close=1&constrained=1&email=example%"
      "40domain.com&validateEmail=1&readOnlyEmail=1");
  EXPECT_EQ(expected_url_1,
            GetReauthURLWithEmailForDialog(
                signin_metrics::AccessPoint::ACCESS_POINT_START_PAGE,
                signin_metrics::Reason::REASON_SIGNIN_PRIMARY_ACCOUNT,
                "example@domain.com"));
}

}  // namespace signin
