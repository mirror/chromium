// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/net/cookies/wk_http_system_cookie_store.h"
#import <Foundation/Foundation.h>
#import <WebKit/Webkit.h>

#include "ios/net/cookies/system_cookie_store_unittest.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(__IPHONE_11_0) && (__IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_11_0)

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace net {

// Test class that conforms to net::SystemCookieStoreTestDelegate to exercise
// WKHTTPSystemCookieStore.
class API_AVAILABLE(ios(11.0)) WKHTTPSystemCookieStoreTestDelegate {
 public:
  WKHTTPSystemCookieStoreTestDelegate()
      : shared_store_(WKWebsiteDataStore.defaultDataStore.httpCookieStore),
        store_(base::MakeUnique<web::WKHTTPSystemCookieStore>(shared_store_)) {}
  ~WKHTTPSystemCookieStoreTestDelegate() = default;

  void VerifyCookieStatus(NSHTTPCookie* system_cookie,
                          NSURL* url,
                          bool is_set) {
    // Verify that cookie is set in system storage.
    __block bool is_set_result = false;
    __block bool verification_done = false;
    [shared_store_ getAllCookies:^(NSArray<NSHTTPCookie*>* cookies) {
      NSHTTPCookie* result_cookie = nil;
      for (NSHTTPCookie* cookie in cookies) {
        if ([cookie.name isEqualToString:system_cookie.name]) {
          result_cookie = cookie;
          break;
        }
      }
      is_set_result = [result_cookie.value isEqualToString:system_cookie.value];
      verification_done = true;
    }];
    base::test::ios::WaitUntilCondition(^bool {
      return verification_done;
    });
    EXPECT_EQ(is_set, is_set_result);
  }

  void ClearCookies() {
    __block int cookies_found = -1;
    __block int cookies_deleted = 0;
    [shared_store_ getAllCookies:^(NSArray<NSHTTPCookie*>* cookies) {
      cookies_found = cookies.count;
      for (NSHTTPCookie* cookie in cookies) {
        [shared_store_ deleteCookie:(NSHTTPCookie*)cookie
                  completionHandler:^{
                    cookies_deleted++;
                  }];
      }
    }];
    base::test::ios::WaitUntilCondition(^bool {
      return cookies_found == cookies_deleted;
    });
  }

  void VerifyCookiesCount(int count) {
    __block int cookies_count = -1;
    [shared_store_ getAllCookies:^(NSArray<NSHTTPCookie*>* cookies) {
      cookies_count = cookies.count;
    }];
    base::test::ios::WaitUntilCondition(^bool {
      return cookies_count > -1;
    });
    EXPECT_EQ(count, cookies_count);
  }

  SystemCookieStore* GetCookieStore() { return store_.get(); }

 private:
  web::TestWebThreadBundle web_thread_;
  WKHTTPCookieStore* shared_store_;
  std::unique_ptr<web::WKHTTPSystemCookieStore> store_;
};

API_AVAILABLE(ios(11.0))
INSTANTIATE_TYPED_TEST_CASE_P(WKHTTPSystemCookieStore,
                              SystemCookieStoreTest,
                              WKHTTPSystemCookieStoreTestDelegate);

}  // namespace net

#endif
