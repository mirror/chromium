// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/net/cookies/ns_http_system_cookie_store.h"

#import <Foundation/Foundation.h>

#include <memory>

#include "base/memory/ptr_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace net {

// Test fixture to exercise net::NSHTTPSystemCookieStore created with
// |[NSHTTPCookieStorage sharedHTTPCookieStorage]|.
class NSHTTPSystemCookieStoreTest : public testing::Test {
 public:
  NSHTTPSystemCookieStoreTest()
      : shared_store_([NSHTTPCookieStorage sharedHTTPCookieStorage]),
        store_(base::MakeUnique<net::NSHTTPSystemCookieStore>(shared_store_)),
        kTestCookieURL([NSURL URLWithString:@"http://foo.google.com/bar"]) {}

  void ClearCookies() {
    [shared_store_ setCookieAcceptPolicy:NSHTTPCookieAcceptPolicyAlways];
    NSArray* cookies = shared_store_.cookies;
    for (NSHTTPCookie* cookie in cookies)
      [shared_store_ deleteCookie:cookie];
    EXPECT_EQ(0u, shared_store_.cookies.count);
  }

  void SetUp() override { ClearCookies(); }

  ~NSHTTPSystemCookieStoreTest() override {}

 protected:
  NSHTTPCookieStorage* shared_store_;
  std::unique_ptr<net::NSHTTPSystemCookieStore> store_;
  NSURL* kTestCookieURL;
};

TEST_F(NSHTTPSystemCookieStoreTest, SetCookie) {
  NSString* cookie_name = @"a";
  NSString* cookie_value = @"b";
  NSString* cookie_line = @"a=b";
  NSHTTPCookie* system_cookie = [[NSHTTPCookie
      cookiesWithResponseHeaderFields:@{@"Set-Cookie" : cookie_line}
                               forURL:kTestCookieURL] objectAtIndex:0];
  // Verify that cookie is not set in system storage.
  for (NSHTTPCookie* cookie in [shared_store_ cookiesForURL:kTestCookieURL]) {
    EXPECT_FALSE([cookie.name isEqualToString:cookie_name]);
  }
  store_->SetCookie(system_cookie);

  // Verify that cookie is set in system storage.
  NSHTTPCookie* result_cookie = nil;

  for (NSHTTPCookie* cookie in [shared_store_ cookiesForURL:kTestCookieURL]) {
    if ([cookie.name isEqualToString:cookie_name]) {
      result_cookie = cookie;
      break;
    }
  }
  EXPECT_TRUE([result_cookie.value isEqualToString:cookie_value]);
}

TEST_F(NSHTTPSystemCookieStoreTest, ClearCookies) {
  NSHTTPCookie* system_cookie = [[NSHTTPCookie
      cookiesWithResponseHeaderFields:@{@"Set-Cookie" : @"a=b"}
                               forURL:kTestCookieURL] objectAtIndex:0];
  [shared_store_ setCookie:system_cookie];

  system_cookie = [[NSHTTPCookie
      cookiesWithResponseHeaderFields:@{@"Set-Cookie" : @"x=d"}
                               forURL:[NSURL
                                          URLWithString:@"http://bar.xyz.abc"]]
      objectAtIndex:0];
  [shared_store_ setCookie:system_cookie];
  EXPECT_EQ(2u, shared_store_.cookies.count);
  store_->ClearStore();
  EXPECT_EQ(0u, shared_store_.cookies.count);
}

}  // namespace net
