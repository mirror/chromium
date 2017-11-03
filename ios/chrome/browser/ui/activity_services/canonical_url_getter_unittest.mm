// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/activity_services/canonical_url_getter.h"

#import <Foundation/Foundation.h>

#include "base/macros.h"
#import "ios/chrome/browser/ui/activity_services/canonical_url_getter_delegate.h"
#import "ios/web/public/test/web_test_with_web_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Test object which exposes state of a CanonicalURLGetterDelegate.
@interface TestCanonicalURLGetterDelegate : NSObject<CanonicalURLGetterDelegate>

// Indicates that the delegate was notified of a retrieved |canonicalURL|.
@property(nonatomic, assign) BOOL canonicalURLReceived;

// The canonical URL retrieved by the CanonicalURLGetter. If no canonical URL
// has been retrieved, then this property is an empty GURL.
@property(nonatomic, assign) GURL canonicalURL;
@end

@implementation TestCanonicalURLGetterDelegate

@synthesize canonicalURLReceived = _canonicalURLReceived;
@synthesize canonicalURL = _canonicalURL;

- (instancetype)init {
  self = [super init];
  if (self) {
    _canonicalURL = GURL();
    _canonicalURLReceived = NO;
  }
  return self;
}

- (void)canonicalURLGetter:(CanonicalURLGetter*)getter
    didRetrieveCanonicalURL:(const GURL&)canonicalURL {
  self.canonicalURL = canonicalURL;
  self.canonicalURLReceived = YES;
}

@end

// Test fixture for testing the CanonicalURLGetter.
class CanonicalURLGetterTest : public web::WebTestWithWebState {
 public:
  CanonicalURLGetterTest() = default;
  ~CanonicalURLGetterTest() override = default;

 protected:
  void SetUp() override {
    web::WebTestWithWebState::SetUp();
    delegate_ = [[TestCanonicalURLGetterDelegate alloc] init];
    getter_ = [[CanonicalURLGetter alloc] init];
    getter_.delegate = delegate_;
  }

  // Returns a CanonicalURLGetter to be used in tests.
  CanonicalURLGetter* getter() { return getter_; }
  // Returns the CanonicalURLGetterDelegate to be used in tests.
  TestCanonicalURLGetterDelegate* delegate() { return delegate_; }

 private:
  __strong CanonicalURLGetter* getter_;
  __strong TestCanonicalURLGetterDelegate* delegate_;
  DISALLOW_COPY_AND_ASSIGN(CanonicalURLGetterTest);
};

// Validates that if there is one canonical URL, it is found and given to the
// delegate.
TEST_F(CanonicalURLGetterTest, TestOneCanonicalURLFound) {
  LoadHtml(@"<link rel=\"canonical\" href=\"http://chromium.test\">",
           GURL("http://m.chromium.test/"));

  [getter() retrieveCanonicalURLForWebState:web_state()];
  TestCanonicalURLGetterDelegate* getterDelegate = delegate();
  WaitForCondition(^{
    return static_cast<bool>(getterDelegate.canonicalURLReceived);
  });

  EXPECT_EQ("http://chromium.test/", getterDelegate.canonicalURL);
}

// Validates that if there is no canonical URL, an empty GURL is given to the
// delegate.
TEST_F(CanonicalURLGetterTest, TestNoCanonicalURLFound) {
  LoadHtml(@"No canonical link on this page.");

  [getter() retrieveCanonicalURLForWebState:web_state()];
  TestCanonicalURLGetterDelegate* getterDelegate = delegate();
  WaitForCondition(^{
    return static_cast<bool>(getterDelegate.canonicalURLReceived);
  });

  EXPECT_TRUE(getterDelegate.canonicalURL.is_empty());
}

// Validates that if the found canonical URL is invalid, an empty GURL is
// given to the delegate.
TEST_F(CanonicalURLGetterTest, TestInvalidCanonicalFound) {
  LoadHtml(@"<link rel=\"canonical\" href=\"chromium\">");

  [getter() retrieveCanonicalURLForWebState:web_state()];
  TestCanonicalURLGetterDelegate* getterDelegate = delegate();
  WaitForCondition(^{
    return static_cast<bool>(getterDelegate.canonicalURLReceived);
  });

  EXPECT_TRUE(getterDelegate.canonicalURL.is_empty());
}

// Validates that if multiple canonical URLs are found, the first one is given
// to the delegate.
TEST_F(CanonicalURLGetterTest, TestMultipleCanonicalURLsFound) {
  LoadHtml(
      @"<link rel=\"canonical\" href=\"http://chromium.test\">"
      @"<link rel=\"canonical\" href=\"http://chromium1.test\">",
      GURL("http://m.chromium.test/"));

  [getter() retrieveCanonicalURLForWebState:web_state()];
  TestCanonicalURLGetterDelegate* getterDelegate = delegate();
  WaitForCondition(^{
    return static_cast<bool>(getterDelegate.canonicalURLReceived);
  });

  EXPECT_EQ("http://chromium.test/", getterDelegate.canonicalURL);
}

// Validates that if both the visible and canonical URLs are HTTPS, the
// canonical URL is given to the delegate.
TEST_F(CanonicalURLGetterTest, TestCanonicalURLHTTPS) {
  LoadHtml(@"<link rel=\"canonical\" href=\"https://chromium.test\">",
           GURL("https://m.chromium.test/"));

  [getter() retrieveCanonicalURLForWebState:web_state()];
  TestCanonicalURLGetterDelegate* getterDelegate = delegate();
  WaitForCondition(^{
    return static_cast<bool>(getterDelegate.canonicalURLReceived);
  });

  EXPECT_EQ("https://chromium.test/", getterDelegate.canonicalURL);
}

// Validates that if the visible URL is HTTP but the canonical URL is HTTPS, the
// canonical URL is given to the delegate.
TEST_F(CanonicalURLGetterTest, TestCanonicalURLHTTPSUpgrade) {
  LoadHtml(@"<link rel=\"canonical\" href=\"https://chromium.test\">",
           GURL("http://m.chromium.test/"));

  [getter() retrieveCanonicalURLForWebState:web_state()];
  TestCanonicalURLGetterDelegate* getterDelegate = delegate();
  WaitForCondition(^{
    return static_cast<bool>(getterDelegate.canonicalURLReceived);
  });

  EXPECT_EQ("https://chromium.test/", getterDelegate.canonicalURL);
}

// Validates that if the visible URL is HTTPS but the canonical URL is HTTP, an
// empty GURL is given to the delegate.
TEST_F(CanonicalURLGetterTest, TestCanonicalLinkHTTPSDowngrade) {
  LoadHtml(@"<link rel=\"canonical\" href=\"http://chromium.test\">",
           GURL("https://m.chromium.test/"));

  [getter() retrieveCanonicalURLForWebState:web_state()];
  TestCanonicalURLGetterDelegate* getterDelegate = delegate();
  WaitForCondition(^{
    return static_cast<bool>(getterDelegate.canonicalURLReceived);
  });

  EXPECT_TRUE(getterDelegate.canonicalURL.is_empty());
}
