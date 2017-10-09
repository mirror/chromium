// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/fonts/WebFontTypefaceFactory.h"

#include "build/build_config.h"
#include "platform/Histogram.h"
#include "platform/fonts/FontCache.h"
#include "platform/fonts/opentype/FontFormatCheck.h"
#include "platform/wtf/Assertions.h"
#include "third_party/skia/include/core/SkStream.h"
#include "third_party/skia/include/core/SkTypeface.h"
#if defined(OS_WIN) || defined(OS_MACOSX)
#include "third_party/skia/include/ports/SkFontMgr_empty.h"
#endif
#if defined(OS_MACOSX)
#include "platform/fonts/mac/CoreTextVariationsSupport.h"
#endif

namespace blink {

bool WebFontTypefaceFactory::CreateTypeface(sk_sp<SkData> sk_data,
                                            sk_sp<SkTypeface>& typeface) {
  CHECK(!typeface);
  std::unique_ptr<SkStreamAsset> stream(new SkMemoryStream(sk_data));
  sk_sp<SkTypeface> probe_typeface;
#if defined(OS_WIN)
  probe_typeface = sk_sp<SkTypeface>(
      FontCache::GetFontCache()->FontManager()->makeFromStream(
          std::move(stream)));
#else
  probe_typeface =
      sk_sp<SkTypeface>(SkTypeface::MakeFromStream(stream.release()));
#endif

  if (!probe_typeface)
    return false;

  CHECK(probe_typeface);

  FontFormatCheck format_check(probe_typeface);

  if (!format_check.IsVariableFont() && !format_check.IsCbdtCblcColorFont() &&
      !format_check.IsCff2OutlineFont()) {
    ReportWebFontInstantiationResult(kSuccessConventionalWebFont);
    typeface = probe_typeface;
    return true;
  }

  if (format_check.IsCbdtCblcColorFont()) {
    typeface = FreeTypeFontManager()->makeFromStream(
        probe_typeface->openStream(nullptr)->duplicate());
    if (typeface)
      ReportWebFontInstantiationResult(kSuccessCbdtCblcColorFont);
  }

  if (format_check.IsCff2OutlineFont()) {
    typeface = FreeTypeFontManager()->makeFromStream(
        probe_typeface->openStream(nullptr)->duplicate());
    if (typeface)
      ReportWebFontInstantiationResult(kSuccessCff2Font);
  }

  if (format_check.IsVariableFont()) {
    typeface = FontManagerForVariations()->makeFromStream(
        probe_typeface->openStream(nullptr)->duplicate());
    if (typeface)
      ReportWebFontInstantiationResult(kSuccessVariableWebFont);
    else
      ReportWebFontInstantiationResult(kErrorInstantiatingVariableFont);
  }

  // Reset to the conventional default font manager based typeface if
  // instantiation in the fallback font manager did not succeed.
  if (!typeface)
    typeface = probe_typeface;

  return true;
}

sk_sp<SkFontMgr> WebFontTypefaceFactory::FontManagerForVariations() {
#if defined(OS_WIN)
  return FreeTypeFontManager();
#else
#if defined(OS_MACOSX)
  if (!CoreTextVersionSupportsVariations())
    return FreeTypeFontManager();
#endif
  return DefaultFontManager();
#endif
}

sk_sp<SkFontMgr> WebFontTypefaceFactory::DefaultFontManager() {
  return sk_sp<SkFontMgr>(SkFontMgr::RefDefault());
}

sk_sp<SkFontMgr> WebFontTypefaceFactory::FreeTypeFontManager() {
  return sk_sp<SkFontMgr>(SkFontMgr_New_Custom_Empty());
}

void WebFontTypefaceFactory::ReportWebFontInstantiationResult(
    WebFontInstantiationResult result) {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(
      EnumerationHistogram, web_font_variable_fonts_ratio,
      ("Blink.Fonts.VariableFontsRatio", kMaxWebFontInstantiationResult));
  web_font_variable_fonts_ratio.Count(result);
}

}  // namespace blink
