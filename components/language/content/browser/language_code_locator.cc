// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/content/browser/language_code_locator.h"

#include "base/strings/string_split.h"
#include "third_party/s2cellid/src/s2/s2cellid.h"
#include "third_party/s2cellid/src/s2/s2latlng.h"

namespace language {
namespace internal {
extern std::vector<std::pair<uint32_t, char>> GenerateDistrictLanguageMapping();
extern std::vector<std::pair<char, std::string>>
GenerateLanguageEnumCodeMapping();
}  // namespace internal

LanguageCodeLocator::LanguageCodeLocator()
    : district_languages_(internal::GenerateDistrictLanguageMapping()),
      language_code_enums_(internal::GenerateLanguageEnumCodeMapping()) {}

LanguageCodeLocator::~LanguageCodeLocator() {}

std::vector<std::string> LanguageCodeLocator::GetLanguageCode(
    double latitude,
    double longitude) const {
  S2CellId current_cell(S2LatLng::FromDegrees(latitude, longitude));
  while (current_cell.level() > 0) {
    auto search = district_languages_.find(current_cell.id() >> 32);
    if (search != district_languages_.end()) {
      auto language_code_search = language_code_enums_.find(search->second);
      if (language_code_search != language_code_enums_.end()) {
        return base::SplitString(language_code_search->second, ";",
                                 base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
      }
      return {};
    }
    current_cell = current_cell.parent();
  }
  return {};
}

}  // namespace language
