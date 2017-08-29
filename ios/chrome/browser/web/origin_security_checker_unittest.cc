// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/web/origin_security_checker.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

const char kHttpsUrl[] = "https://foo.test/";
const char kHttpUrl[] = "http://foo.test/";
const char kLocalhostUrl[] = "http://localhost";
const char kDataUrl[] = "data://foo.test/";

namespace web {

// Test IsOriginSecureAndNonData function.
TEST(OriginSecurityCheckerTest, SecureOriginWithNonDataScheme) {
  EXPECT_TRUE(IsOriginSecureAndNonData(GURL(kHttpsUrl)));
  // Localhost is a secure origin.
  EXPECT_TRUE(IsOriginSecureAndNonData(GURL(kLocalhostUrl)));
  EXPECT_FALSE(IsOriginSecureAndNonData(GURL(kHttpUrl)));
  EXPECT_FALSE(IsOriginSecureAndNonData(GURL(kDataUrl)));
}

}  // namespace web
