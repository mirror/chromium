// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/fonts/FontGlobalContext.h"

#include "platform/Language.h"
#include "platform/fonts/FontCache.h"
#include "platform/wtf/Locker.h"
#include "platform/wtf/StdLibExtras.h"
#include "platform/wtf/ThreadSpecific.h"
#include "platform/wtf/ThreadingPrimitives.h"

namespace blink {

AtomicString& GlobalPlatformLanguage() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(AtomicString, platform_language, ());
  return platform_language;
}

FontGlobalContext* FontGlobalContext::Get(CreateIfNeeded create_if_needed) {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(ThreadSpecific<FontGlobalContext*>,
                                  font_persistent, ());
  if (!*font_persistent && create_if_needed == kCreate) {
    *font_persistent = new FontGlobalContext();
  }
  return *font_persistent;
}

void FontGlobalContext::SetPlatformLanguage(const AtomicString& language) {
  Get()->platform_language_ = language;
}

FontGlobalContext::FontGlobalContext()
    : platform_language_(
          GlobalPlatformLanguage().Impl()
              ? GlobalPlatformLanguage().Impl()->IsolatedCopy().Get()
              : nullptr) {}

void FontGlobalContext::ClearMemory() {
  if (!Get(kDoNotCreate))
    return;

  GetFontCache().Invalidate();
}

void FontGlobalContext::Initialize() {
  AtomicString& global_lang = GlobalPlatformLanguage();
  global_lang = AtomicString(DefaultLanguage().Impl()->IsolatedCopy().Get());
}

}  // namespace blink
