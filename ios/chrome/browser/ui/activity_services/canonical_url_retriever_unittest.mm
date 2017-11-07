// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/activity_services/canonical_url_retriever.h"

#import <Foundation/Foundation.h>

#include "base/macros.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/test/web_test_with_web_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Test fixture for the retrieving canonical URLs.
class CanonicalURLGetterTest : public web::WebTestWithWebState {
 public:
  CanonicalURLGetterTest() = default;
  ~CanonicalURLGetterTest() override = default;

 protected:
  // Retrieves the canonical URL and returns it through the |url| out parameter.
  // Returns whether the operation was successful.
  bool RetrieveCanonicalUrl(GURL* url) {
    __block GURL result;
    __block bool canonical_url_received = false;
    activity_services::RetrieveCanonicalUrl(web_state(), ^(const GURL& url) {
      result = url;
      canonical_url_received = true;
    });

    bool success = testing::WaitUntilConditionOrTimeout(
        testing::kWaitForJSCompletionTimeout, ^{
          return canonical_url_received;
        });

    *url = result;
    return success;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CanonicalURLGetterTest);
};

// Validates that if there is one canonical URL, it is found and given to the
// completion block.
TEST_F(CanonicalURLGetterTest, TestOneCanonicalURLFound) {
  LoadHtml(@"<link rel=\"canonical\" href=\"http://chromium.test\">",
           GURL("http://m.chromium.test/"));

  GURL url;
  bool success = RetrieveCanonicalUrl(&url);

  ASSERT_TRUE(success);
  EXPECT_EQ("http://chromium.test/", url);
}

// Validates that if there is no canonical URL, an empty GURL is given to the
// completion block.
TEST_F(CanonicalURLGetterTest, TestNoCanonicalURLFound) {
  LoadHtml(@"No canonical link on this page.");

  GURL url;
  bool success = RetrieveCanonicalUrl(&url);

  ASSERT_TRUE(success);
  EXPECT_TRUE(url.is_empty());
}

// Validates that if the found canonical URL is invalid, an empty GURL is
// given to the completion block.
TEST_F(CanonicalURLGetterTest, TestInvalidCanonicalFound) {
  LoadHtml(@"<link rel=\"canonical\" href=\"chromium\">");

  GURL url;
  bool success = RetrieveCanonicalUrl(&url);

  ASSERT_TRUE(success);
  EXPECT_TRUE(url.is_empty());
}

// Validates that if multiple canonical URLs are found, the first one is given
// to the completion block.
TEST_F(CanonicalURLGetterTest, TestMultipleCanonicalURLsFound) {
  LoadHtml(
      @"<link rel=\"canonical\" href=\"http://chromium.test\">"
      @"<link rel=\"canonical\" href=\"http://chromium1.test\">",
      GURL("http://m.chromium.test/"));

  GURL url;
  bool success = RetrieveCanonicalUrl(&url);

  ASSERT_TRUE(success);
  EXPECT_EQ("http://chromium.test/", url);
}

// Validates that if both the visible and canonical URLs are HTTPS, the
// canonical URL is given to the completion block.
TEST_F(CanonicalURLGetterTest, TestCanonicalURLHTTPS) {
  LoadHtml(@"<link rel=\"canonical\" href=\"https://chromium.test\">",
           GURL("https://m.chromium.test/"));

  GURL url;
  bool success = RetrieveCanonicalUrl(&url);

  ASSERT_TRUE(success);
  EXPECT_EQ("https://chromium.test/", url);
}

// Validates that if the visible URL is HTTP but the canonical URL is HTTPS, the
// canonical URL is given to the completion block.
TEST_F(CanonicalURLGetterTest, TestCanonicalURLHTTPSUpgrade) {
  LoadHtml(@"<link rel=\"canonical\" href=\"https://chromium.test\">",
           GURL("http://m.chromium.test/"));

  GURL url;
  bool success = RetrieveCanonicalUrl(&url);

  ASSERT_TRUE(success);
  EXPECT_EQ("https://chromium.test/", url);
}

// Validates that if the visible URL is HTTPS but the canonical URL is HTTP, an
// empty GURL is given to the completion block.
TEST_F(CanonicalURLGetterTest, TestCanonicalLinkHTTPSDowngrade) {
  LoadHtml(@"<link rel=\"canonical\" href=\"http://chromium.test\">",
           GURL("https://m.chromium.test/"));

  GURL url;
  bool success = RetrieveCanonicalUrl(&url);

  ASSERT_TRUE(success);
  EXPECT_TRUE(url.is_empty());
}
