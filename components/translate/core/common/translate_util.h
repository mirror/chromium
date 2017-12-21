// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_TRANSLATE_CORE_COMMON_TRANSLATE_UTIL_H_
#define COMPONENTS_TRANSLATE_CORE_COMMON_TRANSLATE_UTIL_H_

#include <string>
#include <vector>

class GURL;

namespace translate {

// Isolated world sets following security-origin by default.
extern const char kSecurityOrigin[];

// Converts language code synonym to use at Translate server.
//
// The same logic exists at language_options.js, and please keep consistency
// with the JavaScript file.
void ToTranslateLanguageSynonym(std::string* language);

// Converts language code synonym to use at Chrome internal.
void ToChromeLanguageSynonym(std::string* language);

// Gets Security origin with which Translate runs. This is used both for
// language checks and to obtain the list of available languages.
GURL GetTranslateSecurityOrigin();

std::string ExtractBaseLanguage(const std::string& language_code);

// Returns whether or not the given list includes at least a language with the
// same base as the input language.
// For example: "en-US" and "en-UK" share the same base "en".
bool ContainsSameBaseLanguage(const std::vector<std::string>& list,
                              const std::string& language_code);

// Gets the actual UI locale that Chrome should use for display and returns
// whether such locale exist.
// If |input_locale| cannot be used as display UI, this method returns false
// and the content of |actual_locale| is not modified.
// This method should be called whenever the display locale preference is
// read, because users can selected a set of languages that is larger than
// the set of actual UI locales.
bool GetActualUILocale(const std::string& input_locale,
                       std::string* actual_locale);

}  // namespace translate

#endif  // COMPONENTS_TRANSLATE_CORE_COMMON_TRANSLATE_UTIL_H_
