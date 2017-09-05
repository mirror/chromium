// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/net/cookies/nshttp_system_cookie_store.h"

#import <Foundation/Foundation.h>

#include <memory>

#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace net {

namespace {

// Test fixture to exercise net::NSHTTPSystemCookieStore created with
// |[NSHTTPCookieStorage sharedHTTPCookieStorage]|.
class NSHTTPSystemCookieStoreTest : public testing::Test {
 public:
  NSHTTPSystemCookieStoreTest()
      : shared_store_([NSHTTPCookieStorage sharedHTTPCookieStorage]),
        store_(base::MakeUnique<net::NSHTTPSystemCookieStore>(
            shared_store_.get())),
        kTestCookieURL([NSURL URLWithString:@"http://foo.google.com/bar"]) {}

  void ClearCookies() {
    [shared_store_ setCookieAcceptPolicy:NSHTTPCookieAcceptPolicyAlways];
    NSArray* cookies = [shared_store_ cookies];
    for (NSHTTPCookie* cookie in cookies)
      [shared_store_ deleteCookie:cookie];
    EXPECT_EQ(0u, [[shared_store_ cookies] count]);
  }

  void SetUp() override { ClearCookies(); }

  ~NSHTTPSystemCookieStoreTest() override {}

 protected:
  base::MessageLoop loop_;
  base::scoped_nsobject<NSHTTPCookieStorage> shared_store_;
  std::unique_ptr<net::NSHTTPSystemCookieStore> store_;
  NSURL* kTestCookieURL;
};

}  // namespace

TEST_F(NSHTTPSystemCookieStoreTest, SetCookie) {
  NSString* cookieName = @"a";
  NSString* cookieValue = @"b";
  NSString* cookieLine = @"a=b";
  NSHTTPCookie* systemCookie = [[NSHTTPCookie
      cookiesWithResponseHeaderFields:@{@"Set-Cookie" : cookieLine}
                               forURL:kTestCookieURL] objectAtIndex:0];
  // Verify that cookie is not set in system storage.
  for (NSHTTPCookie* cookie in [shared_store_ cookiesForURL:kTestCookieURL]) {
    EXPECT_FALSE([[cookie name] isEqualToString:cookieName]);
  }
  store_->SetCookie(systemCookie);

  // Verify that cookie is set in system storage.
  NSHTTPCookie* resultCookie = nil;

  for (NSHTTPCookie* cookie in [shared_store_ cookiesForURL:kTestCookieURL]) {
    if ([cookie.name isEqualToString:cookieName]) {
      resultCookie = cookie;
      break;
    }
  }
  EXPECT_TRUE([[resultCookie value] isEqualToString:cookieValue]);
}

TEST_F(NSHTTPSystemCookieStoreTest, ClearCookies) {
  NSHTTPCookie* systemCookie = [[NSHTTPCookie
      cookiesWithResponseHeaderFields:@{@"Set-Cookie" : @"a=b"}
                               forURL:kTestCookieURL] objectAtIndex:0];
  [shared_store_ setCookie:systemCookie];

  systemCookie = [[NSHTTPCookie
      cookiesWithResponseHeaderFields:@{@"Set-Cookie" : @"x=d"}
                               forURL:[NSURL
                                          URLWithString:@"http://bar.xyz.abc"]]
      objectAtIndex:0];
  [shared_store_ setCookie:systemCookie];
  EXPECT_EQ(2u, [[shared_store_ cookies] count]);
  store_->ClearStore();
  EXPECT_EQ(0u, [[shared_store_ cookies] count]);
}

}  // namespace net
