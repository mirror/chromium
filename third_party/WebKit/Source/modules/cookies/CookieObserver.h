// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CookieObserver_h
#define CookieObserver_h

#include "platform/instrumentation/tracing/TracedValue.h"

namespace blink {

class LocalFrame;

class CookieObserver {
 public:
  CookieObserver(LocalFrame*);
  ~CookieObserver();
  static CookieObserver* Create(LocalFrame*);

  DECLARE_TRACE();

 private:
  void Dispose();

  Member<LocalFrame> frame_;
};

}  // namespace blink

#endif
