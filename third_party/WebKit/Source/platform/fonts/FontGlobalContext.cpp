// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/fonts/FontGlobalContext.h"

#include "platform/Language.h"
#include "platform/fonts/AcceptLanguagesResolver.h"
#include "platform/fonts/FontCache.h"
#include "platform/wtf/StdLibExtras.h"
#include "platform/wtf/ThreadSpecific.h"

#include <unicode/locid.h>

namespace blink {

FontGlobalContext* FontGlobalContext::Get(CreateIfNeeded create_if_needed) {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(ThreadSpecific<FontGlobalContext*>,
                                  font_persistent, ());
  if (!*font_persistent && create_if_needed == kCreate) {
    *font_persistent = new FontGlobalContext();
  }
  return *font_persistent;
}

FontGlobalContext::FontGlobalContext()
    : default_locale_(nullptr),
      system_locale_(nullptr),
      default_locale_for_han_(nullptr),
      harfbuzz_font_funcs_(nullptr) {}

const LayoutLocale& FontGlobalContext::GetDefaultLayoutLocale() {
  FontGlobalContext* ctx = Get();
  if (ctx->default_locale_)
    return *ctx->default_locale_;

  AtomicString locale = DefaultLanguage();
  ctx->default_locale_ = LayoutLocale::Get(!locale.IsEmpty() ? locale : "en");
  return *ctx->default_locale_;
}

const LayoutLocale& FontGlobalContext::GetSystemLayoutLocale() {
  FontGlobalContext* ctx = Get();
  if (ctx->system_locale_)
    return *ctx->system_locale_;

  // Platforms such as Windows can give more information than the default
  // locale, such as "en-JP" for English speakers in Japan.
  String name = icu::Locale::getDefault().getName();
  ctx->system_locale_ = LayoutLocale::Get(AtomicString(name.Replace('_', '-')));
  return *ctx->system_locale_;
}

const LayoutLocale& FontGlobalContext::GetDefaultLocaleForHan() {
  FontGlobalContext* ctx = Get();
  if (ctx->default_locale_for_han_)
    return *ctx->default_locale_for_han_;

  if (const LayoutLocale* locale = AcceptLanguagesResolver::LocaleForHan())
    ctx->default_locale_for_han_ = locale;
  else if (GetDefaultLayoutLocale().HasScriptForHan())
    ctx->default_locale_for_han_ = &FontGlobalContext::GetDefaultLayoutLocale();
  else if (GetSystemLayoutLocale().HasScriptForHan())
    ctx->default_locale_for_han_ = &FontGlobalContext::GetSystemLayoutLocale();
  else
    ctx->default_locale_for_han_ = nullptr;
  return *ctx->default_locale_for_han_;
}

void FontGlobalContext::ClearDefaultLocaleForHan() {
  Get()->default_locale_for_han_ = nullptr;
}

void FontGlobalContext::ClearMemory() {
  if (!Get(kDoNotCreate))
    return;

  GetFontCache().Invalidate();
}

void FontGlobalContext::ClearForTesting() {
  FontGlobalContext* ctx = Get();
  ctx->default_locale_ = nullptr;
  ctx->system_locale_ = nullptr;
  ctx->default_locale_for_han_ = nullptr;
  ctx->layout_locale_map_.clear();
}

}  // namespace blink
