// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/net/cookies/cookie_store_ios.h"

#import <Foundation/Foundation.h>

#include <memory>

#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/sys_string_conversions.h"
#import "ios/net/cookies/cookie_store_ios_test_util.h"
#import "net/base/mac/url_conversions.h"
#include "net/cookies/cookie_store_unittest.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace net {

struct CookieStoreIOSTestTraits {
  static std::unique_ptr<net::CookieStore> Create() {
    ClearCookies();
    return base::MakeUnique<CookieStoreIOS>(
        [NSHTTPCookieStorage sharedHTTPCookieStorage]);
  }

  static const bool supports_http_only = false;
  static const bool supports_non_dotted_domains = false;
  static const bool preserves_trailing_dots = false;
  static const bool filters_schemes = false;
  static const bool has_path_prefix_bug = true;
  static const bool forbids_setting_empty_name = true;
  static const int creation_time_granularity_in_ms = 1000;

  base::MessageLoop loop_;
};

INSTANTIATE_TYPED_TEST_CASE_P(CookieStoreIOS,
                              CookieStoreTest,
                              CookieStoreIOSTestTraits);


namespace {

// Helper callback to be passed to CookieStore::GetAllCookiesForURLAsync().
class GetAllCookiesCallback {
 public:
  GetAllCookiesCallback() : did_run_(false) {}

  // Returns true if the callback has been run.
  bool did_run() { return did_run_; }

  // Returns the parameter of the callback.
  const net::CookieList& cookie_list() { return cookie_list_; }

  void Run(const net::CookieList& cookie_list) {
    ASSERT_FALSE(did_run_);
    did_run_ = true;
    cookie_list_ = cookie_list;
  }

 private:
  bool did_run_;
  net::CookieList cookie_list_;
};

void IgnoreBoolean(bool ignored) {
}

void IgnoreString(const std::string& ignored) {
}

}  // namespace

// Test fixture to exercise net::CookieStoreIOS created without backend and
// synchronized with |[NSHTTPCookieStorage sharedHTTPCookieStorage]|.
class CookieStoreIOSTest : public testing::Test {
 public:
  CookieStoreIOSTest()
      : kTestCookieURL_foo_bar("http://foo.google.com/bar"),
        kTestCookieURL_foo_baz("http://foo.google.com/baz"),
        kTestCookieURL_foo("http://foo.google.com"),
        kTestCookieURL_bar_bar("http://bar.google.com/bar"),
        backend_(new TestPersistentCookieStore),
        store_(base::MakeUnique<net::CookieStoreIOS>(
            [NSHTTPCookieStorage sharedHTTPCookieStorage])) {
    ClearCookies();
    cookie_changed_callback_ = store_->AddCallbackForCookie(
        kTestCookieURL_foo_bar, "abc",
        base::Bind(&RecordCookieChanges, &cookies_changed_, &cookies_removed_));
  }

  ~CookieStoreIOSTest() override {}

  // Gets the cookies. |callback| will be called on completion.
  void GetCookies(net::CookieStore::GetCookiesCallback callback) {
    net::CookieOptions options;
    options.set_include_httponly();
    store_->GetCookiesWithOptionsAsync(kTestCookieURL_foo_bar, options,
                                       std::move(callback));
  }

  // Sets a cookie.
  void SetCookie(const std::string& cookie_line) {
    net::SetCookie(cookie_line, kTestCookieURL_foo_bar, store_.get());
  }

  void SetSystemCookie(const GURL& url,
                       const std::string& name,
                       const std::string& value) {
    NSHTTPCookieStorage* storage =
        [NSHTTPCookieStorage sharedHTTPCookieStorage];
    [storage setCookie:[NSHTTPCookie cookieWithProperties:@{
      NSHTTPCookiePath : base::SysUTF8ToNSString(url.path()),
      NSHTTPCookieName : base::SysUTF8ToNSString(name),
      NSHTTPCookieValue : base::SysUTF8ToNSString(value),
      NSHTTPCookieDomain : base::SysUTF8ToNSString(url.host()),
    }]];
    net::CookieStoreIOS::NotifySystemCookiesChanged();
    base::RunLoop().RunUntilIdle();
  }

  void DeleteSystemCookie(const GURL& gurl, const std::string& name) {
    NSHTTPCookieStorage* storage =
        [NSHTTPCookieStorage sharedHTTPCookieStorage];
    NSURL* nsurl = net::NSURLWithGURL(gurl);
    NSArray* cookies = [storage cookiesForURL:nsurl];
    for (NSHTTPCookie* cookie in cookies) {
      if (cookie.name.UTF8String == name) {
        [storage deleteCookie:cookie];
        break;
      }
    }
    net::CookieStoreIOS::NotifySystemCookiesChanged();
    base::RunLoop().RunUntilIdle();
  }

 protected:
  const GURL kTestCookieURL_foo_bar;
  const GURL kTestCookieURL_foo_baz;
  const GURL kTestCookieURL_foo;
  const GURL kTestCookieURL_bar_bar;

  base::MessageLoop loop_;
  scoped_refptr<TestPersistentCookieStore> backend_;
  std::unique_ptr<net::CookieStoreIOS> store_;
  std::unique_ptr<net::CookieStore::CookieChangedSubscription>
      cookie_changed_callback_;
  std::vector<net::CanonicalCookie> cookies_changed_;
  std::vector<bool> cookies_removed_;
};

TEST_F(CookieStoreIOSTest, SetCookieCallsHookWhenSynchronized) {
  GetCookies(base::Bind(&IgnoreString));
  ClearCookies();
  SetCookie("abc=def");
  EXPECT_EQ(1U, cookies_changed_.size());
  EXPECT_EQ(1U, cookies_removed_.size());
  EXPECT_EQ("abc", cookies_changed_[0].Name());
  EXPECT_EQ("def", cookies_changed_[0].Value());
  EXPECT_FALSE(cookies_removed_[0]);

  SetCookie("abc=ghi");
  EXPECT_EQ(3U, cookies_changed_.size());
  EXPECT_EQ(3U, cookies_removed_.size());
  EXPECT_EQ("abc", cookies_changed_[1].Name());
  EXPECT_EQ("def", cookies_changed_[1].Value());
  EXPECT_TRUE(cookies_removed_[1]);
  EXPECT_EQ("abc", cookies_changed_[2].Name());
  EXPECT_EQ("ghi", cookies_changed_[2].Value());
  EXPECT_FALSE(cookies_removed_[2]);
  DeleteSystemCookie(kTestCookieURL_foo_bar, "abc");
}

TEST_F(CookieStoreIOSTest, DeleteCallsHook) {
  GetCookies(base::Bind(&IgnoreString));
  ClearCookies();
  SetCookie("abc=def");
  EXPECT_EQ(1U, cookies_changed_.size());
  EXPECT_EQ(1U, cookies_removed_.size());
  store_->DeleteCookieAsync(kTestCookieURL_foo_bar, "abc",
                            base::Bind(&IgnoreBoolean, false));
  CookieStoreIOS::NotifySystemCookiesChanged();
  base::RunLoop().RunUntilIdle();
}

TEST_F(CookieStoreIOSTest, SameValueDoesNotCallHook) {
  GetCookieCallback callback;
  GetCookies(base::Bind(&IgnoreString));
  ClearCookies();
  SetCookie("abc=def");
  EXPECT_EQ(1U, cookies_changed_.size());
  SetCookie("abc=def");
  EXPECT_EQ(1U, cookies_changed_.size());
}

TEST(CookieStoreIOS, GetAllCookiesForURLAsync) {
  base::MessageLoop loop;
  const GURL kTestCookieURL_foo_bar("http://foo.google.com/bar");
  ClearCookies();
  std::unique_ptr<CookieStoreIOS> cookie_store(base::MakeUnique<CookieStoreIOS>(
      [NSHTTPCookieStorage sharedHTTPCookieStorage]));

  // Add a cookie.
  net::CookieOptions options;
  options.set_include_httponly();
  cookie_store->SetCookieWithOptionsAsync(
      kTestCookieURL_foo_bar, "a=b", options,
      net::CookieStore::SetCookiesCallback());
  // Check we can get the cookie.
  GetAllCookiesCallback callback;
  cookie_store->GetAllCookiesForURLAsync(
      kTestCookieURL_foo_bar,
      base::Bind(&GetAllCookiesCallback::Run, base::Unretained(&callback)));
  EXPECT_TRUE(callback.did_run());
  EXPECT_EQ(1u, callback.cookie_list().size());
  net::CanonicalCookie cookie = callback.cookie_list()[0];
  EXPECT_EQ("a", cookie.Name());
  EXPECT_EQ("b", cookie.Value());
}

TEST_F(CookieStoreIOSTest, NoInitialNotifyWithNoCookie) {
  std::vector<net::CanonicalCookie> cookies;
  store_->AddCallbackForCookie(
      kTestCookieURL_foo_bar, "abc",
      base::Bind(&RecordCookieChanges, &cookies, nullptr));
  EXPECT_EQ(0U, cookies.size());
}

TEST_F(CookieStoreIOSTest, NoInitialNotifyWithSystemCookie) {
  SetSystemCookie(kTestCookieURL_foo_bar, "abc", "def");
  std::vector<net::CanonicalCookie> cookies;
  store_->AddCallbackForCookie(
      kTestCookieURL_foo_bar, "abc",
      base::Bind(&RecordCookieChanges, &cookies, nullptr));
  EXPECT_EQ(0U, cookies.size());
  DeleteSystemCookie(kTestCookieURL_foo_bar, "abc");
}

TEST_F(CookieStoreIOSTest, NotifyOnAdd) {
  std::vector<net::CanonicalCookie> cookies;
  std::vector<bool> removes;
  std::unique_ptr<net::CookieStore::CookieChangedSubscription> handle =
      store_->AddCallbackForCookie(
          kTestCookieURL_foo_bar, "abc",
          base::Bind(&RecordCookieChanges, &cookies, &removes));
  EXPECT_EQ(0U, cookies.size());
  EXPECT_EQ(0U, removes.size());
  SetSystemCookie(kTestCookieURL_foo_bar, "abc", "def");
  EXPECT_EQ(1U, cookies.size());
  EXPECT_EQ(1U, removes.size());
  EXPECT_EQ("abc", cookies[0].Name());
  EXPECT_EQ("def", cookies[0].Value());
  EXPECT_FALSE(removes[0]);

  SetSystemCookie(kTestCookieURL_foo_bar, "ghi", "jkl");
  EXPECT_EQ(1U, cookies.size());
  EXPECT_EQ(1U, removes.size());

  DeleteSystemCookie(kTestCookieURL_foo_bar, "abc");
  DeleteSystemCookie(kTestCookieURL_foo_bar, "ghi");
}

TEST_F(CookieStoreIOSTest, NotifyOnChange) {
  std::vector<net::CanonicalCookie> cookies;
  std::vector<bool> removes;
  std::unique_ptr<net::CookieStore::CookieChangedSubscription> handle =
      store_->AddCallbackForCookie(
          kTestCookieURL_foo_bar, "abc",
          base::Bind(&RecordCookieChanges, &cookies, &removes));
  EXPECT_EQ(0U, cookies.size());
  SetSystemCookie(kTestCookieURL_foo_bar, "abc", "def");
  EXPECT_EQ(1U, cookies.size());
  SetSystemCookie(kTestCookieURL_foo_bar, "abc", "ghi");
  EXPECT_EQ(3U, cookies.size());
  EXPECT_EQ(3U, removes.size());
  EXPECT_EQ("abc", cookies[1].Name());
  EXPECT_EQ("def", cookies[1].Value());
  EXPECT_TRUE(removes[1]);
  EXPECT_EQ("abc", cookies[2].Name());
  EXPECT_EQ("ghi", cookies[2].Value());
  EXPECT_FALSE(removes[2]);

  DeleteSystemCookie(kTestCookieURL_foo_bar, "abc");
}

TEST_F(CookieStoreIOSTest, NotifyOnDelete) {
  std::vector<net::CanonicalCookie> cookies;
  std::vector<bool> removes;
  SetSystemCookie(kTestCookieURL_foo_bar, "abc", "def");
  std::unique_ptr<net::CookieStore::CookieChangedSubscription> handle =
      store_->AddCallbackForCookie(
          kTestCookieURL_foo_bar, "abc",
          base::Bind(&RecordCookieChanges, &cookies, &removes));
  EXPECT_EQ(0U, cookies.size());
  DeleteSystemCookie(kTestCookieURL_foo_bar, "abc");
  EXPECT_EQ(1U, cookies.size());
  EXPECT_EQ(1U, removes.size());
  EXPECT_TRUE(removes[0]);
  SetSystemCookie(kTestCookieURL_foo_bar, "abc", "def");
  EXPECT_EQ(2U, cookies.size());
  EXPECT_EQ(2U, removes.size());
  EXPECT_FALSE(removes[1]);
  DeleteSystemCookie(kTestCookieURL_foo_bar, "abc");
}

TEST_F(CookieStoreIOSTest, NoNotifyOnNoChange) {
  std::vector<net::CanonicalCookie> cookies;
  std::unique_ptr<net::CookieStore::CookieChangedSubscription> handle =
      store_->AddCallbackForCookie(
          kTestCookieURL_foo_bar, "abc",
          base::Bind(&RecordCookieChanges, &cookies, nullptr));
  EXPECT_EQ(0U, cookies.size());
  SetSystemCookie(kTestCookieURL_foo_bar, "abc", "def");
  EXPECT_EQ(1U, cookies.size());
  SetSystemCookie(kTestCookieURL_foo_bar, "abc", "def");
  EXPECT_EQ(1U, cookies.size());
  DeleteSystemCookie(kTestCookieURL_foo_bar, "abc");
}

TEST_F(CookieStoreIOSTest, MultipleNotifies) {
  ClearCookies();

  std::vector<net::CanonicalCookie> cookies;
  std::vector<net::CanonicalCookie> cookies2;
  std::vector<net::CanonicalCookie> cookies3;
  std::vector<net::CanonicalCookie> cookies4;
  std::unique_ptr<net::CookieStore::CookieChangedSubscription> handle =
      store_->AddCallbackForCookie(
          kTestCookieURL_foo_bar, "abc",
          base::Bind(&RecordCookieChanges, &cookies, nullptr));
  std::unique_ptr<net::CookieStore::CookieChangedSubscription> handle2 =
      store_->AddCallbackForCookie(
          kTestCookieURL_foo_baz, "abc",
          base::Bind(&RecordCookieChanges, &cookies2, nullptr));
  std::unique_ptr<net::CookieStore::CookieChangedSubscription> handle3 =
      store_->AddCallbackForCookie(
          kTestCookieURL_foo, "abc",
          base::Bind(&RecordCookieChanges, &cookies3, nullptr));
  std::unique_ptr<net::CookieStore::CookieChangedSubscription> handle4 =
      store_->AddCallbackForCookie(
          kTestCookieURL_bar_bar, "abc",
          base::Bind(&RecordCookieChanges, &cookies4, nullptr));
  SetSystemCookie(kTestCookieURL_foo_bar, "abc", "def");
  SetSystemCookie(kTestCookieURL_foo_baz, "abc", "def");
  SetSystemCookie(kTestCookieURL_foo, "abc", "def");
  SetSystemCookie(kTestCookieURL_bar_bar, "abc", "def");
  EXPECT_EQ(2U, cookies.size());
  EXPECT_EQ(2U, cookies2.size());
  EXPECT_EQ(1U, cookies3.size());
  EXPECT_EQ(1U, cookies4.size());
  DeleteSystemCookie(kTestCookieURL_foo_bar, "abc");
  DeleteSystemCookie(kTestCookieURL_foo_baz, "abc");
  DeleteSystemCookie(kTestCookieURL_foo, "abc");
  DeleteSystemCookie(kTestCookieURL_bar_bar, "abc");
}

TEST_F(CookieStoreIOSTest, LessSpecificNestedCookie) {
  std::vector<net::CanonicalCookie> cookies;
  SetSystemCookie(kTestCookieURL_foo_baz, "abc", "def");
  std::unique_ptr<net::CookieStore::CookieChangedSubscription> handle =
      store_->AddCallbackForCookie(
          kTestCookieURL_foo_baz, "abc",
          base::Bind(&RecordCookieChanges, &cookies, nullptr));
  EXPECT_EQ(0U, cookies.size());
  SetSystemCookie(kTestCookieURL_foo, "abc", "ghi");
  EXPECT_EQ(1U, cookies.size());
  DeleteSystemCookie(kTestCookieURL_foo_bar, "abc");
}

TEST_F(CookieStoreIOSTest, MoreSpecificNestedCookie) {
  ClearCookies();
  std::vector<net::CanonicalCookie> cookies;
  SetSystemCookie(kTestCookieURL_foo, "abc", "def");
  std::unique_ptr<net::CookieStore::CookieChangedSubscription> handle =
      store_->AddCallbackForCookie(
          kTestCookieURL_foo_baz, "abc",
          base::Bind(&RecordCookieChanges, &cookies, nullptr));
  EXPECT_EQ(0U, cookies.size());
  SetSystemCookie(kTestCookieURL_foo_baz, "abc", "ghi");
  EXPECT_EQ(1U, cookies.size());
  DeleteSystemCookie(kTestCookieURL_foo_bar, "abc");
}

TEST_F(CookieStoreIOSTest, MoreSpecificNestedCookieWithSameValue) {
  std::vector<net::CanonicalCookie> cookies;
  SetSystemCookie(kTestCookieURL_foo, "abc", "def");
  std::unique_ptr<net::CookieStore::CookieChangedSubscription> handle =
      store_->AddCallbackForCookie(
          kTestCookieURL_foo_baz, "abc",
          base::Bind(&RecordCookieChanges, &cookies, nullptr));
  EXPECT_EQ(0U, cookies.size());
  SetSystemCookie(kTestCookieURL_foo_baz, "abc", "def");
  EXPECT_EQ(1U, cookies.size());
  DeleteSystemCookie(kTestCookieURL_foo_bar, "abc");
}

TEST_F(CookieStoreIOSTest, RemoveCallback) {
  std::vector<net::CanonicalCookie> cookies;
  SetSystemCookie(kTestCookieURL_foo_bar, "abc", "def");
  std::unique_ptr<net::CookieStore::CookieChangedSubscription> handle =
      store_->AddCallbackForCookie(
          kTestCookieURL_foo_bar, "abc",
          base::Bind(&RecordCookieChanges, &cookies, nullptr));
  EXPECT_EQ(0U, cookies.size());
  SetSystemCookie(kTestCookieURL_foo_bar, "abc", "ghi");
  EXPECT_EQ(2U, cookies.size());
  // this deletes the callback
  handle.reset();
  SetSystemCookie(kTestCookieURL_foo_bar, "abc", "jkl");
  EXPECT_EQ(2U, cookies.size());
  DeleteSystemCookie(kTestCookieURL_foo_bar, "abc");
}

}  // namespace net
