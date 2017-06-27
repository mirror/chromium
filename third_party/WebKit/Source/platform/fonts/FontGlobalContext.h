// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FontGlobalContext_h
#define FontGlobalContext_h

#include "platform/PlatformExport.h"
#include "platform/fonts/FontCache.h"
#include "platform/wtf/HashMap.h"
#include "platform/wtf/StdLibExtras.h"
#include "platform/wtf/text/AtomicStringHash.h"

namespace blink {

class LayoutLocale;
class FontCache;

using LayoutLocaleMap =
    HashMap<AtomicString, RefPtr<LayoutLocale>, CaseFoldingHash>;

enum CreateIfNeeded { kDoNotCreate, kCreate };

// FontGlobalContext contains non-thread-safe, thread-specific data used for
// font formatting.
class PLATFORM_EXPORT FontGlobalContext {
  WTF_MAKE_NONCOPYABLE(FontGlobalContext);

 public:
  static FontGlobalContext* Get(CreateIfNeeded = kCreate);

  static inline FontCache& GetFontCache() { return Get()->font_cache_; }

  static inline LayoutLocaleMap& GetLayoutLocaleMap() {
    return Get()->layout_locale_map_;
  }

  static const LayoutLocale& GetDefaultLayoutLocale();
  static const LayoutLocale& GetSystemLayoutLocale();
  static const LayoutLocale& GetDefaultLocaleForHan();
  static void ClearDefaultLocaleForHan();

  // Called by MemoryCoordinator to clear memory.
  static void ClearMemory();

  static void ClearForTesting();

 private:
  friend class WTF::ThreadSpecific<FontGlobalContext>;

  FontGlobalContext();

  FontCache font_cache_;

  LayoutLocaleMap layout_locale_map_;
  const LayoutLocale* default_locale_;
  const LayoutLocale* system_locale_;
  const LayoutLocale* default_locale_for_han_;
};

}  // namespace blink

#endif
