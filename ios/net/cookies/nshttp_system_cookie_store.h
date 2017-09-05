// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_NET_COOKIES_NSHTTP_SYSTEM_COOKIE_STORE_H_
#define IOS_NET_COOKIES_NSHTTP_SYSTEM_COOKIE_STORE_H_

#include "base/mac/scoped_nsobject.h"
#include "ios/net/cookies/system_cookie_store.h"

namespace net {

// This class is an implementation of SystemCookieStore, It uses
// NSHTTPCookieStorage as the underlying system cookie store.
class NSHTTPSystemCookieStore : public net::SystemCookieStore {
 public:
  // By default the underlying cookiestore is
  // |NSHTTPCookieStorage sharedHTTPCookieStorage|
  NSHTTPSystemCookieStore();
  explicit NSHTTPSystemCookieStore(NSHTTPCookieStorage* cookie_store);
  ~NSHTTPSystemCookieStore() override;
  NSArray* GetCookiesForURL(const GURL& url,
                            CookieCreationTimeManager* manager) override;
  NSArray* GetAllCookies(CookieCreationTimeManager* manager) override;
  void DeleteCookie(NSHTTPCookie* cookie) override;
  void SetCookie(NSHTTPCookie* cookie) override;
  void ClearStore() override;
  NSHTTPCookieAcceptPolicy GetCookieAcceptPolicy() override;

 private:
  base::scoped_nsobject<NSHTTPCookieStorage> cookie_store_;
};

}  // namespace net

#endif  // IOS_NET_COOKIES_NSHTTP_SYSTEM_COOKIE_STORE_H_
