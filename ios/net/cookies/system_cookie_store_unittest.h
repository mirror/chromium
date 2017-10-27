// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_NET_COOKIES_SYSTEM_COOKIE_STORE_UNITTEST_H_
#define IOS_NET_COOKIES_SYSTEM_COOKIE_STORE_UNITTEST_H_

#import "ios/net/cookies/system_cookie_store.h"

#import <Foundation/Foundation.h>

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/ios/wait_util.h"
#import "ios/net/cookies/cookie_store_ios_test_util.h"
#import "net/base/mac/url_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "url/gurl.h"

namespace net {
namespace {

// Helper callbacks to be passed to SetCookieAsync/GetCookiesAsync.
class SystemCookiesTestCallback {
 public:
  SystemCookiesTestCallback()
      : did_run_with_cookies_(false), did_run_with_no_cookies_(false) {}
  // Returns if the callback has been run.
  bool did_run_with_cookies() { return did_run_with_cookies_; }
  bool did_run_with_no_cookies() { return did_run_with_no_cookies_; }

  // Returns the paremeter of the callback.
  NSArray<NSHTTPCookie*>* cookies() { return cookies_; }

  void RunWithCookies(NSArray<NSHTTPCookie*>* cookies) {
    ASSERT_FALSE(did_run_with_cookies_);
    did_run_with_cookies_ = true;
    cookies_ = cookies;
  }

  void RunWithNoCookies() {
    ASSERT_FALSE(did_run_with_no_cookies_);
    did_run_with_no_cookies_ = true;
  }

 private:
  bool did_run_with_cookies_;
  bool did_run_with_no_cookies_;
  NSArray<NSHTTPCookie*>* cookies_;
};

NSHTTPCookie* CreateCookie(NSString* name, NSString* value, NSURL* url) {
  return [NSHTTPCookie cookieWithProperties:@{
    NSHTTPCookiePath : url.path,
    NSHTTPCookieName : name,
    NSHTTPCookieValue : value,
    NSHTTPCookieDomain : url.host,
  }];
}

}  // namespace

// This class defines tests that implementations of SystemCookieStore should
// pass in order to be conformant.
// To use this test, A delegate class should be created for each implementation.
// The delegate class has to implement the following functions:
//   GetCookieStore()
//     Returns a SystemCookieStore implementation object.
//   VerifyCookieStatus(NSHttpCookie cookie, NSURL url , bool is_set)
//     Verifies if |cookie| is set for |url| in the internal cookie store or not
//     based on the value of |is_set|.
//   ClearCookies()
//     Deletes all cookies in the internal cookie store.
//   VerifyCookiesCount(unsigned int count)
//     Verifies if the number of cookies set in the internal cookie store is
//     equal to |count| .
template <typename SystemCookieStoreTestDelegate>
class SystemCookieStoreTest : public PlatformTest {
 public:
  SystemCookieStoreTest()
      : test_cookie_url1_([NSURL URLWithString:@"http://foo.google.com/bar"]),
        test_cookie_url2_([NSURL URLWithString:@"http://bar.xyz.abc"]),
        test_cookie_url3_([NSURL URLWithString:@"http://123.foo.bar"]) {
    ClearCookies();
  }
  SystemCookieStore* GetCookieStore() { return delegate_.GetCookieStore(); }

  void VerifyCookieStatus(NSHTTPCookie* system_cookie,
                          NSURL* url,
                          bool is_set) {
    delegate_.VerifyCookieStatus(system_cookie, url, is_set);
  }

  void ClearCookies() { delegate_.ClearCookies(); }

  void VerifyCookiesCount(unsigned int count) {
    delegate_.VerifyCookiesCount(count);
  }

 protected:
  NSURL* test_cookie_url1_;
  NSURL* test_cookie_url2_;
  NSURL* test_cookie_url3_;

 private:
  SystemCookieStoreTestDelegate delegate_;
};

TYPED_TEST_CASE_P(SystemCookieStoreTest);

TYPED_TEST_P(SystemCookieStoreTest, SetCookieAsync) {
  NSHTTPCookie* system_cookie =
      CreateCookie(@"a", @"b", this->test_cookie_url1_);
  SystemCookiesTestCallback callback;
  SystemCookieStore* cookie_store = this->GetCookieStore();
  cookie_store->SetCookieAsync(
      system_cookie, /*optional_creation_time=*/nullptr,
      base::BindOnce(&SystemCookiesTestCallback::RunWithNoCookies,
                     base::Unretained(&callback)));
  // Test here is bit tricky as some main thread callbacks may be waiting for
  // web thread call backs which means that it needs to
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  base::RunLoop().RunUntilIdle();
  // verify callback.
  EXPECT_TRUE(callback.did_run_with_no_cookies());
  this->VerifyCookieStatus(system_cookie, this->test_cookie_url1_,
                           /*is_set=*/true);
}

TYPED_TEST_P(SystemCookieStoreTest, GetCookiesAsync) {
  SystemCookieStore* cookie_store = this->GetCookieStore();
  NSMutableDictionary* input_cookies = [[NSMutableDictionary alloc] init];
  NSHTTPCookie* system_cookie =
      CreateCookie(@"a", @"b", this->test_cookie_url1_);
  [input_cookies setValue:system_cookie forKey:@"a"];
  cookie_store->SetCookieAsync(system_cookie,
                               /*optional_creation_time=*/nullptr,
                               SystemCookieStore::SystemCookieCallback());
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));

  system_cookie = CreateCookie(@"x", @"d", this->test_cookie_url2_);
  [input_cookies setValue:system_cookie forKey:@"x"];
  cookie_store->SetCookieAsync(system_cookie,
                               /*optional_creation_time=*/nullptr,
                               SystemCookieStore::SystemCookieCallback());
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  system_cookie = CreateCookie(@"l", @"m", this->test_cookie_url3_);
  [input_cookies setValue:system_cookie forKey:@"l"];
  cookie_store->SetCookieAsync(system_cookie,
                               /*optional_creation_time=*/nullptr,
                               SystemCookieStore::SystemCookieCallback());
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  base::RunLoop().RunUntilIdle();
  // Test GetCookieForURLAsync.
  NSHTTPCookie* input_cookie = [input_cookies valueForKey:@"a"];
  SystemCookiesTestCallback callback_for_url;
  cookie_store->GetCookiesForURLAsync(
      GURLWithNSURL(this->test_cookie_url1_),
      base::BindOnce(&SystemCookiesTestCallback::RunWithCookies,
                     base::Unretained(&callback_for_url)));
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_for_url.did_run_with_cookies());
  EXPECT_EQ(1u, callback_for_url.cookies().count);
  NSHTTPCookie* result_cookie = callback_for_url.cookies()[0];
  EXPECT_TRUE([input_cookie.name isEqualToString:result_cookie.name]);
  EXPECT_TRUE([input_cookie.value isEqualToString:result_cookie.value]);

  // Test GetAllCookies
  SystemCookiesTestCallback callback_all_cookies;
  cookie_store->GetAllCookiesAsync(
      base::BindOnce(&SystemCookiesTestCallback::RunWithCookies,
                     base::Unretained(&callback_all_cookies)));
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(callback_all_cookies.did_run_with_cookies());
  NSArray<NSHTTPCookie*>* result_cookies = callback_all_cookies.cookies();
  EXPECT_EQ(3u, result_cookies.count);
  for (NSHTTPCookie* cookie in result_cookies) {
    NSHTTPCookie* existing_cookie = [input_cookies valueForKey:cookie.name];
    EXPECT_TRUE(existing_cookie);
    EXPECT_TRUE([existing_cookie.name isEqualToString:cookie.name]);
    EXPECT_TRUE([existing_cookie.value isEqualToString:cookie.value]);
    EXPECT_TRUE([existing_cookie.domain isEqualToString:cookie.domain]);
  }
}

TYPED_TEST_P(SystemCookieStoreTest, DeleteCookiesAsync) {
  SystemCookieStore* cookie_store = this->GetCookieStore();

  NSHTTPCookie* system_cookie1 =
      CreateCookie(@"a", @"b", this->test_cookie_url1_);
  cookie_store->SetCookieAsync(system_cookie1,
                               /*optional_creation_time=*/nullptr,
                               SystemCookieStore::SystemCookieCallback());
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  NSHTTPCookie* system_cookie2 =
      CreateCookie(@"x", @"d", this->test_cookie_url2_);
  cookie_store->SetCookieAsync(system_cookie2,
                               /*optional_creation_time=*/nullptr,
                               SystemCookieStore::SystemCookieCallback());
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  NSHTTPCookie* system_cookie3 =
      CreateCookie(@"l", @"m", this->test_cookie_url3_);
  cookie_store->SetCookieAsync(system_cookie3,
                               /*optional_creation_time=*/nullptr,
                               SystemCookieStore::SystemCookieCallback());
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  base::RunLoop().RunUntilIdle();
  this->VerifyCookiesCount(3u);
  SystemCookiesTestCallback callback;
  this->VerifyCookieStatus(system_cookie2, this->test_cookie_url2_,
                           /*is_set=*/true);
  cookie_store->DeleteCookieAsync(
      system_cookie2,
      base::BindOnce(&SystemCookiesTestCallback::RunWithNoCookies,
                     base::Unretained(&callback)));
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback.did_run_with_no_cookies());
  this->VerifyCookieStatus(system_cookie2, this->test_cookie_url2_,
                           /*is_set=*/false);
  this->VerifyCookiesCount(2u);

  cookie_store->DeleteCookieAsync(system_cookie1,
                                  SystemCookieStore::SystemCookieCallback());
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  base::RunLoop().RunUntilIdle();
  this->VerifyCookieStatus(system_cookie1, this->test_cookie_url1_,
                           /*is_set=*/false);
  this->VerifyCookiesCount(1u);

  cookie_store->DeleteCookieAsync(system_cookie3,
                                  SystemCookieStore::SystemCookieCallback());
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  base::RunLoop().RunUntilIdle();
  this->VerifyCookiesCount(0u);
}

TYPED_TEST_P(SystemCookieStoreTest, ClearCookiesAsync) {
  SystemCookieStore* cookie_store = this->GetCookieStore();

  cookie_store->SetCookieAsync(
      CreateCookie(@"a", @"b", this->test_cookie_url1_),
      /*optional_creation_time=*/nullptr,
      SystemCookieStore::SystemCookieCallback());
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  cookie_store->SetCookieAsync(
      CreateCookie(@"x", @"d", this->test_cookie_url2_),
      /*optional_creation_time=*/nullptr,
      SystemCookieStore::SystemCookieCallback());
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  base::RunLoop().RunUntilIdle();
  SystemCookiesTestCallback callback;
  this->VerifyCookiesCount(2u);
  cookie_store->ClearStoreAsync(
      base::BindOnce(&SystemCookiesTestCallback::RunWithNoCookies,
                     base::Unretained(&callback)));
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback.did_run_with_no_cookies());
  this->VerifyCookiesCount(0u);
}

TYPED_TEST_P(SystemCookieStoreTest, GetCookieAcceptPolicy) {
  SystemCookieStore* cookie_store = this->GetCookieStore();
  EXPECT_EQ([NSHTTPCookieStorage sharedHTTPCookieStorage].cookieAcceptPolicy,
            cookie_store->GetCookieAcceptPolicy());
  [NSHTTPCookieStorage sharedHTTPCookieStorage].cookieAcceptPolicy =
      NSHTTPCookieAcceptPolicyNever;
  EXPECT_EQ(NSHTTPCookieAcceptPolicyNever,
            cookie_store->GetCookieAcceptPolicy());
  [NSHTTPCookieStorage sharedHTTPCookieStorage].cookieAcceptPolicy =
      NSHTTPCookieAcceptPolicyAlways;
  EXPECT_EQ(NSHTTPCookieAcceptPolicyAlways,
            cookie_store->GetCookieAcceptPolicy());
}

REGISTER_TYPED_TEST_CASE_P(SystemCookieStoreTest,
                           SetCookieAsync,
                           GetCookiesAsync,
                           DeleteCookiesAsync,
                           ClearCookiesAsync,
                           GetCookieAcceptPolicy);

}  // namespace net

#endif  // IOS_NET_COOKIES_SYSTEM_COOKIE_STORE_UNITTEST_H
