// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/fonts/FontGlobalContext.h"

#include "platform/fonts/FontCache.h"
#include "platform/wtf/Locker.h"
#include "platform/wtf/StdLibExtras.h"
#include "platform/wtf/ThreadSpecific.h"
#include "platform/wtf/ThreadingPrimitives.h"

namespace blink {

Mutex& GlobalContextMutex() {
  static Mutex global_context_mutex;
  return global_context_mutex;
}

AtomicString& GlobalPlatformLanguage() {
  static AtomicString platform_language;
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
  MutexLocker locker(GlobalContextMutex());

  Get()->platform_language_ = language;
  AtomicString& global_lang = GlobalPlatformLanguage();
  global_lang = AtomicString(language.Impl()->IsolatedCopy().Get());
}

FontGlobalContext::FontGlobalContext() {
  MutexLocker locker(GlobalContextMutex());
  platform_language_ = GlobalPlatformLanguage();
}

void FontGlobalContext::ClearMemory() {
  if (!Get(kDoNotCreate))
    return;

  GetFontCache().Invalidate();
}

}  // namespace blink
