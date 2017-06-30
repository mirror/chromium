// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/core/browser/translate_language_list.h"

#include <string>
#include <vector>

#include "base/test/scoped_command_line.h"
#include "components/translate/core/browser/translate_download_manager.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace translate {

TEST(TranslateLanguageListTest, SetSupportedLanguages) {
  std::string language_list(
      "{"
      "\"sl\":{\"en\":\"English\",\"ja\":\"Japanese\"},"
      "\"tl\":{\"en\":\"English\",\"ja\":\"Japanese\"}"
      "}");
  TranslateDownloadManager* manager = TranslateDownloadManager::GetInstance();
  manager->set_application_locale("en");
  EXPECT_TRUE(manager->language_list()->SetSupportedLanguages(language_list));

  std::vector<std::string> results;
  manager->language_list()->GetSupportedLanguages(&results);
  ASSERT_EQ(2u, results.size());
  EXPECT_EQ("en", results[0]);
  EXPECT_EQ("ja", results[1]);
  manager->ResetForTesting();
}

TEST(TranslateLanguageListTest, GetLanguageCode) {
  TranslateLanguageList language_list;
  EXPECT_EQ(language_list.GetLanguageCode("en"), "en");
  // Test backoff of unsupported locale.
  EXPECT_EQ(language_list.GetLanguageCode("en-US"), "en");
  // Test supported locale not backed off.
  EXPECT_EQ(language_list.GetLanguageCode("zh-CN"), "zh-CN");
}

TEST(TranslateLanguageListTest, TranslateLanguageUrl) {
  TranslateLanguageList language_list;

  // Test default security origin.
  EXPECT_EQ(language_list.TranslateLanguageUrl().spec(),
            "https://translate.googleapis.com/translate_a/l?client=chrome");

  // Test command-line security origin.
  base::test::ScopedCommandLine scoped_command_line;
  scoped_command_line.GetProcessCommandLine()->AppendSwitchASCII(
      "translate-security-origin", "https://example.com");
  EXPECT_EQ(language_list.TranslateLanguageUrl().spec(),
            "https://example.com/translate_a/l?client=chrome");
}

TEST(TranslateLanguageListTest, IsSupportedLanguage) {
  TranslateLanguageList language_list;
  EXPECT_TRUE(language_list.IsSupportedLanguage("en"));
  EXPECT_TRUE(language_list.IsSupportedLanguage("zh-CN"));
  EXPECT_FALSE(language_list.IsSupportedLanguage("xx"));
}

TEST(TranslateLanguageListTest, GetSupportedLanguages) {
  TranslateLanguageList language_list;
  std::vector<std::string> languages;
  language_list.GetSupportedLanguages(&languages);
  // Check there are a lot of default languages.
  EXPECT_GE(languages.size(), 100ul);
  // Check that some very common languages are there.
  const auto begin = languages.begin();
  const auto end = languages.end();
  EXPECT_NE(std::find(begin, end, "en"), end);
  EXPECT_NE(std::find(begin, end, "es"), end);
  EXPECT_NE(std::find(begin, end, "fr"), end);
  EXPECT_NE(std::find(begin, end, "ru"), end);
  EXPECT_NE(std::find(begin, end, "zh-CN"), end);
  EXPECT_NE(std::find(begin, end, "zh-TW"), end);
}

}  // namespace translate
