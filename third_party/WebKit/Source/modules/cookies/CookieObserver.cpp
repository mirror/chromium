// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/CookieObserver.h"

#include "core/dom/Document.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/LocalFrameClient.h"
#include "public/platform/Platform.h"
#include "public/platform/WebCookieJar.h"
#include "public/platform/WebURL.h"

namespace blink {

CookieObserver::CookieObserver(LocalFrame* frame) : frame_(frame) {
  DCHECK(frame_);
}

DEFINE_TRACE(CookieObserver) {
  visitor->Trace(frame_);
}

CookieObserver::~CookieObserver() {}

static CookieObserver* CookieObserver::Create(LocalFrame* frame) {
  CookieObserver* cookieObserver = new CookieObserver(frame);
  return cookieObserver;
}

}  // namespace blink
