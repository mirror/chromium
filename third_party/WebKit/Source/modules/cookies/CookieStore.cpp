// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/CookieStore.h"

#include "core/dom/Document.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/LocalFrameClient.h"
#include "public/platform/Platform.h"
#include "public/platform/WebCookieJar.h"
#include "public/platform/WebURL.h"

namespace blink {

CookieStore::CookieStore(LocalFrame* frame) : frame_(frame) {
  DCHECK(frame_);
}

DEFINE_TRACE(CookieStore) {
  visitor->Trace(frame_);
}

CookieStore::~CookieStore() {}

static CookieStore* CookieStore::Create(LocalFrame* frame) {
  CookieStore* cookieStore = new CookieStore(frame);
  return cookieStore;
}

}  // namespace blink
