// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FontGlobalContext_h
#define FontGlobalContext_h

#include "platform/PlatformExport.h"
#include "platform/fonts/FontCache.h"
#include "platform/fonts/shaping/HarfBuzzFontCache.h"
#include "platform/wtf/HashMap.h"
#include "platform/wtf/StdLibExtras.h"
#include "platform/wtf/text/AtomicStringHash.h"

struct hb_font_funcs_t;

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
  static inline HarfBuzzFontCache& GetHarfBuzzFontCache() {
    return Get()->harf_buzz_font_cache_;
  }

  static inline LayoutLocaleMap& GetLayoutLocaleMap() {
    return Get()->layout_locale_map_;
  }

  static inline Vector<AtomicString>& GetPreferredLanguagesOverride() {
    return Get()->perferred_languages_override_;
  }

  static const LayoutLocale& GetDefaultLayoutLocale();
  static const LayoutLocale& GetSystemLayoutLocale();
  static const LayoutLocale& GetDefaultLocaleForHan();
  static void ClearDefaultLocaleForHan();

  static inline const AtomicString& GetPlatformLanguage() {
    return Get()->platform_language_;
  }

  static void SetPlatformLanguage(const AtomicString& language) {
    Get()->platform_language_ = language;
  }

  static hb_font_funcs_t* GetHarfBuzzFontFuncs() {
    return Get()->harfbuzz_font_funcs_;
  }

  static void SetHarfBuzzFontFuncs(hb_font_funcs_t* funcs) {
    Get()->harfbuzz_font_funcs_ = funcs;
  }

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

  HarfBuzzFontCache harf_buzz_font_cache_;

  Vector<AtomicString> perferred_languages_override_;
  AtomicString platform_language_;

  hb_font_funcs_t* harfbuzz_font_funcs_;
};

}  // namespace blink

#endif
