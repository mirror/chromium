// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/core/common/translate_util.h"

#include "base/command_line.h"
#include "components/translate/core/common/translate_switches.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

typedef testing::Test TranslateUtilTest;

// Tests that synonym language code is converted to one used in supporting list.
TEST_F(TranslateUtilTest, ToTranslateLanguageSynonym) {
  std::string language;

  language = std::string("nb");
  translate::ToTranslateLanguageSynonym(&language);
  EXPECT_EQ("no", language);

  // Test all known Chinese cases.
  language = std::string("zh-HK");
  translate::ToTranslateLanguageSynonym(&language);
  EXPECT_EQ("zh-TW", language);
  language = std::string("zh-MO");
  translate::ToTranslateLanguageSynonym(&language);
  EXPECT_EQ("zh-TW", language);
  language = std::string("zh-SG");
  translate::ToTranslateLanguageSynonym(&language);
  EXPECT_EQ("zh-CN", language);
  language = std::string("zh");
  translate::ToTranslateLanguageSynonym(&language);
  EXPECT_EQ("zh-CN", language);

  // A sub code is not preserved (except for Chinese).
  language = std::string("he-IL");
  translate::ToTranslateLanguageSynonym(&language);
  EXPECT_EQ("iw", language);

  language = std::string("zh-JP");
  translate::ToTranslateLanguageSynonym(&language);
  EXPECT_EQ("zh-JP", language);

  // Preserve the argument if it doesn't have its synonym.
  language = std::string("en");
  translate::ToTranslateLanguageSynonym(&language);
  EXPECT_EQ("en", language);
}

// Tests that synonym language code is converted to one used in Chrome internal.
TEST_F(TranslateUtilTest, ToChromeLanguageSynonym) {
  std::string language;

  language = std::string("no");
  translate::ToChromeLanguageSynonym(&language);
  EXPECT_EQ("nb", language);

  // Preserve a sub code
  language = std::string("iw-IL");
  translate::ToChromeLanguageSynonym(&language);
  EXPECT_EQ("he-IL", language);

  // Preserve the argument if it doesn't have its synonym.
  language = std::string("en");
  translate::ToChromeLanguageSynonym(&language);
  EXPECT_EQ("en", language);
}

TEST_F(TranslateUtilTest, SecurityOrigin) {
  GURL origin = translate::GetTranslateSecurityOrigin();
  EXPECT_EQ(std::string(translate::kSecurityOrigin), origin.spec());

  const std::string running_origin("http://www.tamurayukari.com/");
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitchASCII(translate::switches::kTranslateSecurityOrigin,
                                  running_origin);
  GURL modified_origin = translate::GetTranslateSecurityOrigin();
  EXPECT_EQ(running_origin, modified_origin.spec());
}

TEST_F(TranslateUtilTest, ContainsSameBaseLanguage) {
  std::vector<std::string> list;

  // Empty input.
  EXPECT_EQ(false, translate::ContainsSameBaseLanguage(list, ""));

  // Empty list.
  EXPECT_EQ(false, translate::ContainsSameBaseLanguage(list, "fr-FR"));

  // Empty language.
  list = {"en-US"};
  EXPECT_EQ(false, translate::ContainsSameBaseLanguage(list, ""));

  // One element, no match.
  list = {"en-US"};
  EXPECT_EQ(false, translate::ContainsSameBaseLanguage(list, "fr-FR"));

  // One element, with match.
  list = {"fr-CA"};
  EXPECT_EQ(true, translate::ContainsSameBaseLanguage(list, "fr-FR"));

  // Multiple elements, no match.
  list = {"en-US", "es-AR", "en-UK"};
  EXPECT_EQ(false, translate::ContainsSameBaseLanguage(list, "fr-FR"));

  // Multiple elements, with match.
  list = {"en-US", "fr-CA", "es-AR"};
  EXPECT_EQ(true, translate::ContainsSameBaseLanguage(list, "fr-FR"));

  // Multiple elements matching.
  list = {"en-US", "fr-CA", "es-AR", "fr-FR"};
  EXPECT_EQ(true, translate::ContainsSameBaseLanguage(list, "fr-FR"));

  // List includes base language.
  list = {"en-US", "fr", "es-AR", "fr-FR"};
  EXPECT_EQ(true, translate::ContainsSameBaseLanguage(list, "fr-FR"));
}

TEST_F(TranslateUtilTest, GetActualUILocale) {
  std::string output;

  //---------------------------------------------------------------------------
  // Languages that are enabled as display UI.
  //---------------------------------------------------------------------------
  bool is_ui = translate::GetActualUILocale("en-US", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("en-US", output);

  is_ui = translate::GetActualUILocale("it", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("it", output);

  is_ui = translate::GetActualUILocale("fr-FR", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("fr-FR", output);

  //---------------------------------------------------------------------------
  // Languages that are converted to their fallback version.
  //---------------------------------------------------------------------------

  // All Latin American Spanish languages fall back to "es-419".
  is_ui = translate::GetActualUILocale("es-AR", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", output);
  is_ui = translate::GetActualUILocale("es-CL", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", output);
  is_ui = translate::GetActualUILocale("es-CO", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", output);
  is_ui = translate::GetActualUILocale("es-CR", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", output);
  is_ui = translate::GetActualUILocale("es-HN", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", output);
  is_ui = translate::GetActualUILocale("es-MX", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", output);
  is_ui = translate::GetActualUILocale("es-PE", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", output);
  is_ui = translate::GetActualUILocale("es-US", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", output);
  is_ui = translate::GetActualUILocale("es-UY", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", output);
  is_ui = translate::GetActualUILocale("es-VE", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", output);

  // English falls back to US.
  is_ui = translate::GetActualUILocale("en", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("en-US", output);

  // All other regional English languages fall back to UK.
  is_ui = translate::GetActualUILocale("en-AU", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("en-GB", output);
  is_ui = translate::GetActualUILocale("en-CA", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("en-GB", output);
  is_ui = translate::GetActualUILocale("en-IN", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("en-GB", output);
  is_ui = translate::GetActualUILocale("en-NZ", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("en-GB", output);
  is_ui = translate::GetActualUILocale("en-ZA", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("en-GB", output);

  is_ui = translate::GetActualUILocale("pt", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("pt-PT", output);

  is_ui = translate::GetActualUILocale("it-CH", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("it", output);

  is_ui = translate::GetActualUILocale("nn", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("nb", output);
  is_ui = translate::GetActualUILocale("no", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("nb", output);

  //---------------------------------------------------------------------------
  // Languages that have their base language is a UI language.
  //---------------------------------------------------------------------------
  is_ui = translate::GetActualUILocale("it-IT", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("it", output);

  is_ui = translate::GetActualUILocale("de-DE", &output);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("de", output);

  //---------------------------------------------------------------------------
  // Languages that cannot be used as display UI.
  //---------------------------------------------------------------------------
  is_ui = translate::GetActualUILocale("af", &output);  // Afrikaans
  EXPECT_FALSE(is_ui);

  is_ui = translate::GetActualUILocale("ga", &output);  // Irish
  EXPECT_FALSE(is_ui);

  is_ui = translate::GetActualUILocale("ky", &output);  // Kyrgyz
  EXPECT_FALSE(is_ui);

  is_ui = translate::GetActualUILocale("sd", &output);  // Sindhi
  EXPECT_FALSE(is_ui);

  is_ui = translate::GetActualUILocale("zu", &output);  // Zulu
  EXPECT_FALSE(is_ui);
}
