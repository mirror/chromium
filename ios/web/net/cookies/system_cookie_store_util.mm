// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/system_cookie_store_util.h"

#import <WebKit/WebKit.h>

#include "base/memory/ptr_util.h"
#import "ios/net/cookies/ns_http_system_cookie_store.h"
#include "ios/net/cookies/system_cookie_store.h"
#import "ios/web/net/cookies/cookie_store_util.h"
#import "ios/web/net/cookies/wk_http_system_cookie_store.h"
#include "ios/web/public/browser_sttate.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

std::unique_ptr<net::SystemCookieStore> CreateSystemCookieStore(
    BrowserState* browser_state) {
  if (@available(iOS 11, *)) {
    WKHTTPCookieStore* wk_cookie_store =
        web::WKCookieStoreForBrowserState(browser_state);
    return base::MakeUnique<web::WKHTTPSystemCookieStore>(wk_cookie_store);
  }
  return base::MakeUnique<net::NSHTTPSystemCookieStore>();
}

}  // namespace web
