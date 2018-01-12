// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/core/common/locale_util.h"

#include <algorithm>
#include <set>
#include <vector>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/strings/string_split.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

// Pair of locales, where the first element should fallback to the second one.
struct LocaleUIFallbackPair {
  const char* const chosen_locale;
  const char* const fallback_locale;
};

// This list MUST be sorted by the first element in the pair, because we perform
// binary search on it.
// TODO(claudiomagni): Norvegian has a bug.
const LocaleUIFallbackPair kLocaleUIFallbackTable[] = {
    {"en", "en-US"},     {"en-AU", "en-GB"},  {"en-CA", "en-GB"},
    {"en-IN", "en-GB"},  {"en-NZ", "en-GB"},  {"en-ZA", "en-GB"},
    {"es-AR", "es-419"}, {"es-CL", "es-419"}, {"es-CO", "es-419"},
    {"es-CR", "es-419"}, {"es-HN", "es-419"}, {"es-MX", "es-419"},
    {"es-PE", "es-419"}, {"es-US", "es-419"}, {"es-UY", "es-419"},
    {"es-VE", "es-419"}, {"it-CH", "it"},     {"nn", "nb"},
    {"no", "nb"},        {"pt", "pt-PT"}};

bool LocaleCompare(const LocaleUIFallbackPair& p1, const std::string& p2) {
  return p1.chosen_locale < p2;
}

bool GetUIFallbackLocale(const std::string& input, std::string* const output) {
  *output = input;
  const auto* it =
      std::lower_bound(std::begin(kLocaleUIFallbackTable),
                       std::end(kLocaleUIFallbackTable), input, LocaleCompare);
  if (it != std::end(kLocaleUIFallbackTable) && it->chosen_locale == input) {
    *output = it->fallback_locale;
    return true;
  }
  return false;
}

// Given a language code, extract the base language only.
// Example: from "en-US", extract "en".
std::string ExtractBaseLanguage(const std::string& language_code) {
  std::string base;
  std::string tail;
  language::SplitIntoMainAndTail(language_code, &base, &tail);
  return base;
}

}  // namespace

namespace language {

void SplitIntoMainAndTail(const std::string& locale,
                          std::string* main_part,
                          std::string* tail_part) {
  DCHECK(main_part);
  DCHECK(tail_part);

  std::vector<base::StringPiece> chunks = base::SplitStringPiece(
      locale, "-", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (chunks.empty())
    return;

  chunks[0].CopyToString(main_part);
  *tail_part = locale.substr(main_part->size());
}

bool ContainsSameBaseLanguage(const std::vector<std::string>& list,
                              const std::string& language_code) {
  std::string base_language;
  std::string unused_str;
  language::SplitIntoMainAndTail(language_code, &base_language, &unused_str);
  for (const auto& item : list) {
    std::string compare_base;
    language::SplitIntoMainAndTail(item, &compare_base, &unused_str);
    if (compare_base == base_language)
      return true;
  }

  return false;
}

bool ConvertToActualUILocale(std::string* input_locale) {
  const std::vector<std::string>& ui_locales = l10n_util::GetAvailableLocales();
  const std::set<std::string> ui_set(ui_locales.begin(), ui_locales.end());

  // 1) Convert input to a fallback, if available.
  std::string fallback;
  if (!GetUIFallbackLocale(*input_locale, &fallback)) {
    fallback = *input_locale;
  }

  // 2) Check if input is part of the UI languages.
  if (ui_set.count(fallback) > 0) {
    *input_locale = fallback;
    return true;
  }

  // 3) Check if the base language of the input is part of the UI languages.
  const std::string base = ExtractBaseLanguage(*input_locale);
  if (base != *input_locale && ui_set.count(base) > 0) {
    *input_locale = base;
    return true;
  }

  return false;
}

}  // namespace language
