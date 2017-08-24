// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/url_utils.h"

#include "content/public/common/browser_side_navigation_policy.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {

TEST(UrlUtilsTest, IsURLHandledByNetworkRequest) {
  if (!IsBrowserSideNavigationEnabled())
    return;

  EXPECT_TRUE(IsURLHandledByNetworkRequest(GURL("http://foo/bar.html")));
  EXPECT_TRUE(IsURLHandledByNetworkRequest(GURL("https://foo/bar.html")));
  EXPECT_TRUE(IsURLHandledByNetworkRequest(GURL("data://foo")));
  EXPECT_TRUE(IsURLHandledByNetworkRequest(GURL("cid:foo@bar")));

  EXPECT_FALSE(IsURLHandledByNetworkRequest(GURL("about:blank")));
  EXPECT_FALSE(IsURLHandledByNetworkRequest(GURL("about:srcdoc")));
  EXPECT_FALSE(IsURLHandledByNetworkRequest(GURL("javascript://foo.js")));
  EXPECT_FALSE(IsURLHandledByNetworkRequest(GURL()));
}

}  // namespace content
