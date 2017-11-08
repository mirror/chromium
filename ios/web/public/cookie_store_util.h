// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_COOKIE_STORE_UTIL_H_
#define IOS_WEB_PUBLIC_COOKIE_STORE_UTIL_H_

#include <memory>

#include "net/cookies/cookie_monster.h"

namespace net {
class CookieStore;
}  // namespace net

namespace web {
class BrowserState;

std::unique_ptr<net::CookieStore> CreateCookieStore(
    net::CookieMonster::PersistentCookieStore* persistent_store,
    BrowserState* browser_state);
}  // namespace web

#endif  // IOS_WEB_PUBLIC_COOKIE_STORE_UTIL_H_
