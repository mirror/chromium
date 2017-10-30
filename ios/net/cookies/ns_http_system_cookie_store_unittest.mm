// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/net/cookies/ns_http_system_cookie_store.h"
#import <Foundation/Foundation.h>

#include "ios/net/cookies/system_cookie_store_unittest_template.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace net {

// Test class that conforms to SystemCookieStoreTestDelegate to exercise
// NSHTTPSystemCookieStore created with
// |[NSHTTPCookieStorage sharedHTTPCookieStorage]|.
class NSHTTPSystemCookieStoreTestDelegate {
 public:
  NSHTTPSystemCookieStoreTestDelegate()
      : scoped_cookie_store_ios_client_(
            base::MakeUnique<TestCookieStoreIOSClient>()),
        shared_store_([NSHTTPCookieStorage sharedHTTPCookieStorage]),
        store_(base::MakeUnique<net::NSHTTPSystemCookieStore>(shared_store_)) {}

  bool IsCookieSet(NSHTTPCookie* system_cookie, NSURL* url) {
    // Verify that cookie is set in system storage.
    NSHTTPCookie* result_cookie = nil;

    for (NSHTTPCookie* cookie in [shared_store_ cookiesForURL:url]) {
      if ([cookie.name isEqualToString:system_cookie.name]) {
        result_cookie = cookie;
        break;
      }
    }
    return [result_cookie.value isEqualToString:system_cookie.value];
  }

  void ClearCookies() {
    // Reset CookieAcceptPolicy to default value.
    [shared_store_ setCookieAcceptPolicy:NSHTTPCookieAcceptPolicyAlways];
    for (NSHTTPCookie* cookie in shared_store_.cookies)
      [shared_store_ deleteCookie:cookie];
    EXPECT_EQ(0u, shared_store_.cookies.count);
  }

  int CookiesCount() { return shared_store_.cookies.count; }

  SystemCookieStore* GetCookieStore() { return store_.get(); }

 private:
  ScopedTestingCookieStoreIOSClient scoped_cookie_store_ios_client_;
  NSHTTPCookieStorage* shared_store_;
  std::unique_ptr<net::NSHTTPSystemCookieStore> store_;
};

INSTANTIATE_TYPED_TEST_CASE_P(NSHTTPSystemCookieStore,
                              SystemCookieStoreTest,
                              NSHTTPSystemCookieStoreTestDelegate);

}  // namespace net
