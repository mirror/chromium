// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/navigation/placeholder_navigation_util.h"

#include "testing/gtest/include/gtest/gtest.h"
//#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

namespace web {
namespace placeholder_navigation_util {

typedef PlatformTest PlaceholderNavigationUtilTest;

TEST_F(PlaceholderNavigationUtilTest, IsPlaceholderUrl) {
  const std::string kPlaceholderUrls[] = {
      "about:blank?for=", "about:blank?for=chrome%3A%2F%2Fnewtab%2F",
  };
  for (const std::string& url : kPlaceholderUrls)
    EXPECT_TRUE(IsPlaceholderUrl(GURL(url)));

  const std::string kNotPlaceholderUrls[] = {
      "", "about:blank", "about:blank?chrome%3A%2F%2Fnewtab%2F",
  };
  for (const std::string& url : kNotPlaceholderUrls)
    EXPECT_FALSE(IsPlaceholderUrl(GURL(url)));
}

// Tests that encoding/decoding is an identity operator for valid URLs.
TEST_F(PlaceholderNavigationUtilTest, EncodeDecodeValidUrls) {
  EXPECT_EQ(GURL("about:blank?for="),
            CreatePlaceholderUrlForUrl(GURL::EmptyGURL()));
  {
    GURL original("chrome://chrome-urls");
    GURL encoded("about:blank?for=chrome%3A%2F%2Fchrome-urls");
    EXPECT_EQ(encoded, CreatePlaceholderUrlForUrl(original));
    EXPECT_EQ(original, ExtractUrlFromPlaceholderUrl(encoded));
  }
  {
    GURL original("about:blank");
    GURL encoded("about:blank?for=about%3Ablank");
    EXPECT_EQ(encoded, CreatePlaceholderUrlForUrl(original));
    EXPECT_EQ(original, ExtractUrlFromPlaceholderUrl(encoded));
  }
}

// Tests that invalid URLs will be rejected in decoding.
TEST_F(PlaceholderNavigationUtilTest, DecodeRejectInvalidUrls) {
  GURL invalid("thisisnotanurl");
  EXPECT_EQ(GURL("about:blank?for="), CreatePlaceholderUrlForUrl(invalid));

  GURL encoded("about:blank?for=thisisnotanurl");
  EXPECT_EQ(GURL::EmptyGURL(), ExtractUrlFromPlaceholderUrl(encoded));
}

}  // namespace placeholder_navigation_util
}  // namespace web
