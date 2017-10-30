// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_NET_COOKIES_SYSTEM_COOKIE_STORE_UNITTEST_TEMPLATE_H_
#define IOS_NET_COOKIES_SYSTEM_COOKIE_STORE_UNITTEST_TEMPLATE_H_

#import "ios/net/cookies/system_cookie_store.h"

#import <Foundation/Foundation.h>

#include <memory>

#include "base/bind.h"
#include "base/callback.h"
#include "base/memory/ptr_util.h"
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
class SystemCookieCallbackRunVerifier {
 public:
  SystemCookieCallbackRunVerifier()
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
//   bool IsCookieSet(NSHttpCookie cookie, NSURL url)
//     Returns wether |cookie| is set for |url| in the internal cookie store or
//     not.
//   ClearCookies()
//     Deletes all cookies in the internal cookie store.
//   int CookiesCount()
//     Returns the number of cookies set in the internal cookie store.
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

  bool IsCookieSet(NSHTTPCookie* system_cookie, NSURL* url) {
    return delegate_.IsCookieSet(system_cookie, url);
  }

  void ClearCookies() { delegate_.ClearCookies(); }

  int CookiesCount() { return delegate_.CookiesCount(); }

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
  SystemCookieCallbackRunVerifier callback;
  SystemCookieStore* cookie_store = this->GetCookieStore();
  cookie_store->SetCookieAsync(
      system_cookie, /*optional_creation_time=*/nullptr,
      base::BindOnce(&SystemCookieCallbackRunVerifier::RunWithNoCookies,
                     base::Unretained(&callback)));
  // Test here is bit tricky as some main thread callbacks may be waiting for
  // web thread call backs.
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  base::RunLoop().RunUntilIdle();
  // verify callback.
  EXPECT_TRUE(callback.did_run_with_no_cookies());
  EXPECT_TRUE(this->IsCookieSet(system_cookie, this->test_cookie_url1_));
}

// Tests cases of GetAllCookiesAsync and GetCookiesForURLAsync.
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
  SystemCookieCallbackRunVerifier callback_for_url;
  cookie_store->GetCookiesForURLAsync(
      GURLWithNSURL(this->test_cookie_url1_),
      base::BindOnce(&SystemCookieCallbackRunVerifier::RunWithCookies,
                     base::Unretained(&callback_for_url)));
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_for_url.did_run_with_cookies());
  EXPECT_EQ(1u, callback_for_url.cookies().count);
  NSHTTPCookie* result_cookie = callback_for_url.cookies()[0];
  EXPECT_TRUE([input_cookie.name isEqualToString:result_cookie.name]);
  EXPECT_TRUE([input_cookie.value isEqualToString:result_cookie.value]);

  // Test GetAllCookies
  SystemCookieCallbackRunVerifier callback_all_cookies;
  cookie_store->GetAllCookiesAsync(
      base::BindOnce(&SystemCookieCallbackRunVerifier::RunWithCookies,
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

// Tests deleting cookies for different URLs and for Different
// cookie key/value pairs.
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
  EXPECT_EQ(3, this->CookiesCount());
  SystemCookieCallbackRunVerifier callback;
  EXPECT_TRUE(this->IsCookieSet(system_cookie2, this->test_cookie_url2_));
  cookie_store->DeleteCookieAsync(
      system_cookie2,
      base::BindOnce(&SystemCookieCallbackRunVerifier::RunWithNoCookies,
                     base::Unretained(&callback)));
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback.did_run_with_no_cookies());
  EXPECT_FALSE(this->IsCookieSet(system_cookie2, this->test_cookie_url2_));
  EXPECT_EQ(2, this->CookiesCount());
  cookie_store->DeleteCookieAsync(system_cookie1,
                                  SystemCookieStore::SystemCookieCallback());
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(this->IsCookieSet(system_cookie1, this->test_cookie_url1_));
  EXPECT_EQ(1, this->CookiesCount());

  cookie_store->DeleteCookieAsync(system_cookie3,
                                  SystemCookieStore::SystemCookieCallback());
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, this->CookiesCount());
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
  SystemCookieCallbackRunVerifier callback;
  EXPECT_EQ(2, this->CookiesCount());
  cookie_store->ClearStoreAsync(
      base::BindOnce(&SystemCookieCallbackRunVerifier::RunWithNoCookies,
                     base::Unretained(&callback)));
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback.did_run_with_no_cookies());
  EXPECT_EQ(0, this->CookiesCount());
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

#endif  // IOS_NET_COOKIES_SYSTEM_COOKIE_STORE_UNITTEST_TEMPLATE_H
