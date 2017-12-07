// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/system_cookie_store_util.h"

#import <WebKit/WebKit.h>

#include "base/memory/ptr_util.h"
#include "base/test/scoped_feature_list.h"
#import "ios/net/cookies/ns_http_system_cookie_store.h"
#include "ios/net/cookies/system_cookie_store.h"
#import "ios/web/net/cookies/wk_http_system_cookie_store.h"
#import "ios/web/public/features.h"
#import "ios/web/public/test/web_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

// Tests that web::CreateSystemCookieStore returns the correct SystemCookieStore
// object based on the iOS version.
using SystemCookieStoreUtilTest = WebTest;
TEST_F(SystemCookieStoreUtilTest, CreateSystemCookieStore) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(base::Feature{
      "WKHTTPSystemCookieStore", base::FEATURE_DISABLED_BY_DEFAULT});
  std::unique_ptr<net::SystemCookieStore> cookie_store =
      web::CreateSystemCookieStore(GetBrowserState());
  if (@available(iOS 11, *)) {
    EXPECT_EQ(net::SystemCookieStore::SystemCookieStoreSyncStatus::
                  kAlwaysInSyncWithWKWebViewCookies,
              cookie_store->GetCookieStoreSyncStatus());
  } else {
    EXPECT_EQ(net::SystemCookieStore::SystemCookieStoreSyncStatus::
                  kBestEffortSyncWithWKWebViewCookies,
              cookie_store->GetCookieStoreSyncStatus());
  }
}

}  // namespace web
