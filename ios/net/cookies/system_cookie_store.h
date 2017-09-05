// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_NET_COOKIES_SYSTEM_COOKIE_STORE_H_
#define IOS_NET_COOKIES_SYSTEM_COOKIE_STORE_H_

#import <Foundation/Foundation.h>

#include "base/macros.h"

class GURL;

namespace net {

class CookieCreationTimeManager;

class SystemCookieStore {
 public:
  SystemCookieStore();
  virtual ~SystemCookieStore();
  virtual NSArray* GetCookiesForURL(const GURL& url);

  // Use the CookieCreateionTimeManager to sort cookies, as per RFC6265.
  virtual NSArray* GetCookiesForURL(const GURL& url,
                                    CookieCreationTimeManager* manager) = 0;
  virtual NSArray* GetAllCookies();

  // Use the CookieCreateionTimeManager to sort cookies, as per RFC6265.
  virtual NSArray* GetAllCookies(CookieCreationTimeManager* manager) = 0;
  virtual void DeleteCookie(NSHTTPCookie* cookie) = 0;
  virtual void SetCookie(NSHTTPCookie* cookie) = 0;
  virtual void ClearStore() = 0;
  virtual NSHTTPCookieAcceptPolicy GetCookieAcceptPolicy() = 0;

 protected:
  // Compares cookies based on the path lengths and the creation times, as per
  // RFC6265.
  static NSInteger CompareCookies(id a, id b, void* context);

 private:
  DISALLOW_COPY_AND_ASSIGN(SystemCookieStore);
};

}  // namespace net

#endif  // IOS_NET_COOKIES_SYSTEM_COOKIE_STORE_H_
