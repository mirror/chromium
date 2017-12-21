// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/core/common/translate_util.h"

#include <stddef.h>
#include <algorithm>
#include <set>
#include <vector>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_split.h"
#include "components/translate/core/common/translate_switches.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace {

// Split the |language| into two parts. For example, if |language| is 'en-US',
// this will be split into the main part 'en' and the tail part '-US'.
void SplitIntoMainAndTail(const std::string& language,
                          std::string* main_part,
                          std::string* tail_part) {
  DCHECK(main_part);
  DCHECK(tail_part);

  std::vector<base::StringPiece> chunks = base::SplitStringPiece(
      language, "-", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (chunks.empty())
    return;

  chunks[0].CopyToString(main_part);
  *tail_part = language.substr(main_part->size());
}

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

}  // namespace

namespace translate {

struct LanguageCodePair {
  // Code used in supporting list of Translate.
  const char* const translate_language;

  // Code used in Chrome internal.
  const char* const chrome_language;
};

// Some languages are treated as same languages in Translate even though they
// are different to be exact.
//
// If this table is updated, please sync this with the synonym table in
// chrome/browser/resources/settings/languages_page/languages.js.
const LanguageCodePair kLanguageCodeSimilitudes[] = {
  {"no", "nb"},
  {"tl", "fil"},
};

// Some languages have changed codes over the years and sometimes the older
// codes are used, so we must see them as synonyms.
//
// If this table is updated, please sync this with the synonym table in
// chrome/browser/resources/settings/languages_page/languages.js.
const LanguageCodePair kLanguageCodeSynonyms[] = {
  {"iw", "he"},
  {"jw", "jv"},
};

// Some Chinese language codes are compatible with zh-TW or zh-CN in terms of
// Translate.
//
// If this table is updated, please sync this with the synonym table in
// chrome/browser/resources/settings/languages_page/languages.js.
const LanguageCodePair kLanguageCodeChineseCompatiblePairs[] = {
    {"zh-TW", "zh-HK"},
    {"zh-TW", "zh-MO"},
    {"zh-CN", "zh-SG"},
    {"zh-CN", "zh"},
};

const char kSecurityOrigin[] = "https://translate.googleapis.com/";

void ToTranslateLanguageSynonym(std::string* language) {
  for (size_t i = 0; i < arraysize(kLanguageCodeSimilitudes); ++i) {
    if (*language == kLanguageCodeSimilitudes[i].chrome_language) {
      *language = kLanguageCodeSimilitudes[i].translate_language;
      return;
    }
  }

  for (size_t i = 0; i < arraysize(kLanguageCodeChineseCompatiblePairs); ++i) {
    if (*language == kLanguageCodeChineseCompatiblePairs[i].chrome_language) {
      *language = kLanguageCodeChineseCompatiblePairs[i].translate_language;
      return;
    }
  }

  std::string main_part, tail_part;
  SplitIntoMainAndTail(*language, &main_part, &tail_part);
  if (main_part.empty())
    return;

  // Chinese is a special case.
  // There is not a single base language, but two: traditional and simplified.
  // The kLanguageCodeChineseCompatiblePairs list contains the relation between
  // various Chinese locales.
  if (main_part == "zh") {
    *language = main_part + tail_part;
    return;
  }

  // Apply linear search here because number of items in the list is just four.
  for (size_t i = 0; i < arraysize(kLanguageCodeSynonyms); ++i) {
    if (main_part == kLanguageCodeSynonyms[i].chrome_language) {
      main_part = std::string(kLanguageCodeSynonyms[i].translate_language);
      break;
    }
  }

  *language = main_part;
}

void ToChromeLanguageSynonym(std::string* language) {
  for (size_t i = 0; i < arraysize(kLanguageCodeSimilitudes); ++i) {
    if (*language == kLanguageCodeSimilitudes[i].translate_language) {
      *language = kLanguageCodeSimilitudes[i].chrome_language;
      return;
    }
  }

  std::string main_part, tail_part;
  SplitIntoMainAndTail(*language, &main_part, &tail_part);
  if (main_part.empty())
    return;

  // Apply liner search here because number of items in the list is just four.
  for (size_t i = 0; i < arraysize(kLanguageCodeSynonyms); ++i) {
    if (main_part == kLanguageCodeSynonyms[i].translate_language) {
      main_part = std::string(kLanguageCodeSynonyms[i].chrome_language);
      break;
    }
  }

  *language = main_part + tail_part;
}

GURL GetTranslateSecurityOrigin() {
  std::string security_origin(kSecurityOrigin);
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kTranslateSecurityOrigin)) {
    security_origin =
        command_line->GetSwitchValueASCII(switches::kTranslateSecurityOrigin);
  }
  return GURL(security_origin);
}

bool ContainsSameBaseLanguage(const std::vector<std::string>& list,
                              const std::string& language_code) {
  std::string base_language;
  std::string unused_str;
  SplitIntoMainAndTail(language_code, &base_language, &unused_str);
  for (const auto& item : list) {
    std::string compare_base;
    SplitIntoMainAndTail(item, &compare_base, &unused_str);
    if (compare_base == base_language)
      return true;
  }

  return false;
}

std::string ExtractBaseLanguage(const std::string& language_code) {
  std::string base;
  std::string tail;
  SplitIntoMainAndTail(language_code, &base, &tail);
  if (!base.empty() && !tail.empty()) {
    return base;
  } else {
    return "";
  }
}

bool GetActualUILocale(const std::string& input_locale,
                       std::string* actual_locale) {
  const std::vector<std::string>& ui_locales = l10n_util::GetAvailableLocales();
  const std::set<std::string> ui_set(ui_locales.begin(), ui_locales.end());

  // 1) Convert to a fallback, if available.
  std::string fallback;
  if (!GetUIFallbackLocale(input_locale, &fallback)) {
    fallback = input_locale;
  }

  // 2) Check if language is part of the UI languages.
  if (ui_set.count(fallback) > 0) {
    *actual_locale = fallback;
    return true;
  }

  // 3) Check if its base is part of the UI languages.
  const std::string base = ExtractBaseLanguage(input_locale);
  if (!base.empty() && ui_set.count(base) > 0) {
    *actual_locale = base;
    return true;
  }

  return false;
}

}  // namespace translate
