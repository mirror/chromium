// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/autocomplete_match_type.h"

#include "base/strings/utf_string_conversions.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(AutocompleteMatchTypeTest, AccessibilityLabelHistory) {
  const base::string16& kTestUrl =
      base::UTF8ToUTF16("https://www.chromium.org");
  const base::string16& kTestTitle = base::UTF8ToUTF16("The Chromium Projects");

  // Test plain url.
  AutocompleteMatch match;
  match.type = AutocompleteMatchType::URL_WHAT_YOU_TYPED;
  match.description = kTestTitle;
  EXPECT_EQ(kTestUrl,
            AutocompleteMatchType::ToAccessibilityLabel(match, kTestUrl));

  // Decorated with title and match type.
  match.type = AutocompleteMatchType::HISTORY_URL;
  EXPECT_EQ(kTestTitle + base::UTF8ToUTF16(" ") + kTestUrl +
                base::UTF8ToUTF16(" location from history"),
            AutocompleteMatchType::ToAccessibilityLabel(match, kTestUrl));
}

TEST(AutocompleteMatchTypeTest, AccessibilityLabelSearch) {
  const base::string16& kSearch = base::UTF8ToUTF16("gondola");
  const base::string16& kSearchDesc = base::UTF8ToUTF16("Google Search");

  AutocompleteMatch match;
  match.type = AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED;
  match.description = kSearchDesc;
  match.type = AutocompleteMatchType::HISTORY_URL;
  EXPECT_EQ(kSearch + base::UTF8ToUTF16(" search"),
            AutocompleteMatchType::ToAccessibilityLabel(match, kSearch));
}
