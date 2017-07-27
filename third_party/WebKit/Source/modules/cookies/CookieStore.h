// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CookieStore_h
#define CookieStore_h

#include "platform/instrumentation/tracing/TracedValue.h"

namespace blink {

class LocalFrame;

class CookieStore {
 public:
  CookieStore(LocalFrame*);
  ~CookieStore();
  static CookieStore* Create(LocalFrame*);

  DECLARE_TRACE();

 private:
  void Dispose();

  Member<LocalFrame> frame_;
};

}  // namespace blink

#endif
