// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/response_headers_util.h"

#include "google_apis/gaia/gaia_urls.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace extensions {

TEST(TestResponseHeadersUtil, ShouldHideResponseHeader) {
  EXPECT_TRUE(ShouldHiderResponseHeader(GaiaUrls::GetInstance()->gaia_url(),
                                        "X-Chrome-ID-Consistency-Response"));
  EXPECT_TRUE(ShouldHiderResponseHeader(GaiaUrls::GetInstance()->gaia_url(),
                                        "x-cHroMe-iD-CoNsiStenCY-RESPoNSE"));
  EXPECT_FALSE(ShouldHiderResponseHeader(GURL("http://www.example.com"),
                                         "X-Chrome-ID-Consistency-Response"));
  EXPECT_FALSE(ShouldHiderResponseHeader(GaiaUrls::GetInstance()->gaia_url(),
                                         "Google-Accounts-SignOut"));
}

}  // namespace extensions
