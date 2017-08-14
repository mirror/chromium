// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GlobalCookieStore_h
#define GlobalCookieStore_h

#include "platform/wtf/Allocator.h"

namespace blink {

class CookieStore;
class LocalDOMWindow;

class GlobalCookieStore {
  STATIC_ONLY(GlobalCookieStore);

 public:
  static CookieStore* cookieStore(LocalDOMWindow&);
};

}  // namespace blink

#endif  // GlobalCookieStore_h
