// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/cookie_store_util.h"

#import <WebKit/WebKit.h>

#include "base/memory/ptr_util.h"
#include "ios/net/cookies/cookie_store_wk_web_view.h"
#import "ios/web/web_state/ui/wk_web_view_configuration_provider.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {
//   main_cookie_store_ = web::CreateCoookieStore(ios_cookie_config,
//   browser_state);

std::uniuqe_ptr<net::CookieStore> CreateCookieStore(
    const CookieStoreConfig& config,
    BrowserState* browser_state) {
  if (config.cookie_store_type == CookieStoreConfig::COOKIE_MONSTER)
    return CreateCookieMonster(config);

  scoped_refptr<net::SQLitePersistentCookieStore> persistent_store = nullptr;
  if (config.session_cookie_mode ==
      CookieStoreConfig::RESTORED_SESSION_COOKIES) {
    DCHECK(!config.path.empty());
    persistent_store = cookie_util::CreatePersistentCookieStore(
        config.path, true /* restore_old_session_cookies */,
        config.crypto_delegate);
  }
  if (base::ios::IsRunningOnIOS11OrLater()) {
    __block WKHTTPCookieStore* system_store = nil;
    // TODO: Find a way to create cookie store on IO thread without blocking it.
    dispatch_sync(dispatch_get_main_queue(), ^{
      auto& config_provider =
          WKWebViewConfigurationProvider::FromBrowserState(browser_state);
      system_store = config_provider.GetWebViewConfiguration()
                         .websiteDataStore.httpCookieStore;
    });
    return base::MakeUnique<net::CookieStoreWKWebView>(persistent_store,
                                                       system_store);
  } else {
    return base::MakeUnique<net::CookieStoreIOSPersistent>(
        persistent_store.get());
  }
}

// Proto
std::unique_ptr<net::CookieStore> CreateCookieStore(
    net::CookieMonster::PersistentCookieStore* persistent_store,
    BrowserState* browser_state) {
  __block WKHTTPCookieStore* system_store = nil;
  // TODO: Find a way to create cookie store on IO thread without blocking it.
  dispatch_sync(dispatch_get_main_queue(), ^{
    auto& config_provider =
        WKWebViewConfigurationProvider::FromBrowserState(browser_state);
    system_store = config_provider.GetWebViewConfiguration()
                       .websiteDataStore.httpCookieStore;
  });
  return base::MakeUnique<net::CookieStoreWKWebView>(persistent_store,
                                                     system_store);
}
// Proto

}  // namespace web
