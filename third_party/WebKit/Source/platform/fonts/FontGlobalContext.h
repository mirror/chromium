// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FontGlobalContext_h
#define FontGlobalContext_h

#include "platform/PlatformExport.h"
#include "platform/fonts/FontCache.h"
#include "platform/wtf/StdLibExtras.h"
#include "platform/wtf/text/AtomicStringHash.h"

namespace blink {

class FontCache;

enum CreateIfNeeded { kDoNotCreate, kCreate };

// FontGlobalContext contains non-thread-safe, thread-specific data used for
// font formatting.
class PLATFORM_EXPORT FontGlobalContext {
  WTF_MAKE_NONCOPYABLE(FontGlobalContext);

 public:
  static FontGlobalContext* Get(CreateIfNeeded = kCreate);

  static inline FontCache& GetFontCache() { return Get()->font_cache_; }

  static inline Vector<AtomicString>& GetPreferredLanguagesOverride() {
    return Get()->perferred_languages_override_;
  }

  static inline const AtomicString& GetPlatformLanguage() {
    return Get()->platform_language_;
  }

  static void SetPlatformLanguage(const AtomicString& language) {
    Get()->platform_language_ = language;
  }

  // Called by MemoryCoordinator to clear memory.
  static void ClearMemory();

 private:
  friend class WTF::ThreadSpecific<FontGlobalContext>;

  FontGlobalContext();

  FontCache font_cache_;

  Vector<AtomicString> perferred_languages_override_;
  AtomicString platform_language_;
};

}  // namespace blink

#endif
