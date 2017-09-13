// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/core/browser/translate_prefs.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "build/build_config.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/translate/core/browser/translate_download_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kTestLanguage[] = "en";

#if defined(OS_CHROMEOS)
const char kPreferredLanguagesPref[] = "settings.language.preferred_languages";
#else
const char* kPreferredLanguagesPref = nullptr;
#endif
const char kAcceptLanguagesPref[] = "intl.accept_languages";

}  // namespace

namespace translate {

class TranslatePrefTest : public testing::Test {
 protected:
  TranslatePrefTest()
      : prefs_(new sync_preferences::TestingPrefServiceSyncable()) {
    translate_prefs_.reset(new translate::TranslatePrefs(
        prefs_.get(), kAcceptLanguagesPref, kPreferredLanguagesPref));
    TranslatePrefs::RegisterProfilePrefs(prefs_->registry());
    now_ = base::Time::Now();
    two_days_ago_ = now_ - base::TimeDelta::FromDays(2);
  }

  void SetUp() override {
    prefs_->registry()->RegisterStringPref(kAcceptLanguagesPref, std::string());
#if defined(OS_CHROMEOS)
    prefs_->registry()->RegisterStringPref(kPreferredLanguagesPref,
                                           std::string());
#endif
  }

  void SetLastDeniedTime(const std::string& language, base::Time time) {
    DenialTimeUpdate update(prefs_.get(), language, 2);
    update.AddDenialTime(time);
  }

  base::Time GetLastDeniedTime(const std::string& language) {
    DenialTimeUpdate update(prefs_.get(), language, 2);
    return update.GetOldestDenialTime();
  }

  std::unique_ptr<sync_preferences::TestingPrefServiceSyncable> prefs_;
  std::unique_ptr<translate::TranslatePrefs> translate_prefs_;

  // Shared time constants.
  base::Time now_;
  base::Time two_days_ago_;
};

TEST_F(TranslatePrefTest, IsTooOftenDeniedIn2016Q2UI) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(translate::kTranslateUI2016Q2);

  translate_prefs_->ResetDenialState();
  EXPECT_FALSE(translate_prefs_->IsTooOftenDenied(kTestLanguage));

  for (int i = 0; i < 3; i++) {
    translate_prefs_->IncrementTranslationDeniedCount(kTestLanguage);
    EXPECT_FALSE(translate_prefs_->IsTooOftenDenied(kTestLanguage));
  }

  translate_prefs_->IncrementTranslationDeniedCount(kTestLanguage);
  EXPECT_TRUE(translate_prefs_->IsTooOftenDenied(kTestLanguage));
}

TEST_F(TranslatePrefTest, IsTooOftenIgnoredIn2016Q2UI) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(translate::kTranslateUI2016Q2);

  translate_prefs_->ResetDenialState();
  EXPECT_FALSE(translate_prefs_->IsTooOftenDenied(kTestLanguage));

  for (int i = 0; i < 10; i++) {
    translate_prefs_->IncrementTranslationIgnoredCount(kTestLanguage);
    EXPECT_FALSE(translate_prefs_->IsTooOftenDenied(kTestLanguage));
  }

  translate_prefs_->IncrementTranslationIgnoredCount(kTestLanguage);
  EXPECT_TRUE(translate_prefs_->IsTooOftenDenied(kTestLanguage));
}

TEST_F(TranslatePrefTest, UpdateLastDeniedTime) {
  // Test that denials with more than 24 hours difference between them do not
  // block the language.
  translate_prefs_->ResetDenialState();
  SetLastDeniedTime(kTestLanguage, two_days_ago_);
  ASSERT_FALSE(translate_prefs_->IsTooOftenDenied(kTestLanguage));
  translate_prefs_->UpdateLastDeniedTime(kTestLanguage);
  base::Time last_denied = GetLastDeniedTime(kTestLanguage);
  EXPECT_FALSE(last_denied.is_max());
  EXPECT_GE(last_denied, now_);
  EXPECT_LT(last_denied - now_, base::TimeDelta::FromSeconds(10));
  EXPECT_FALSE(translate_prefs_->IsTooOftenDenied(kTestLanguage));

  // Ensure the first use simply writes the update time.
  translate_prefs_->ResetDenialState();
  translate_prefs_->UpdateLastDeniedTime(kTestLanguage);
  last_denied = GetLastDeniedTime(kTestLanguage);
  EXPECT_FALSE(last_denied.is_max());
  EXPECT_GE(last_denied, now_);
  EXPECT_LT(last_denied - now_, base::TimeDelta::FromSeconds(10));
  EXPECT_FALSE(translate_prefs_->IsTooOftenDenied(kTestLanguage));

  // If it's denied again within the 24 hour period, language should be
  // permanently denied.
  translate_prefs_->UpdateLastDeniedTime(kTestLanguage);
  last_denied = GetLastDeniedTime(kTestLanguage);
  EXPECT_FALSE(last_denied.is_max());
  EXPECT_GE(last_denied, now_);
  EXPECT_LT(last_denied - now_, base::TimeDelta::FromSeconds(10));
  EXPECT_TRUE(translate_prefs_->IsTooOftenDenied(kTestLanguage));

  // If the language is already permanently denied, don't bother updating the
  // last_denied time.
  ASSERT_TRUE(translate_prefs_->IsTooOftenDenied(kTestLanguage));
  SetLastDeniedTime(kTestLanguage, two_days_ago_);
  translate_prefs_->UpdateLastDeniedTime(kTestLanguage);
  last_denied = GetLastDeniedTime(kTestLanguage);
  EXPECT_EQ(last_denied, two_days_ago_);
}

// Test that the default value for non-existing entries is base::Time::Null().
TEST_F(TranslatePrefTest, DenialTimeUpdate_DefaultTimeIsNull) {
  DenialTimeUpdate update(prefs_.get(), kTestLanguage, 2);
  EXPECT_TRUE(update.GetOldestDenialTime().is_null());
}

// Test that non-existing entries automatically create a ListValue.
TEST_F(TranslatePrefTest, DenialTimeUpdate_ForceListExistence) {
  DictionaryPrefUpdate dict_update(
      prefs_.get(), TranslatePrefs::kPrefTranslateLastDeniedTimeForLanguage);
  base::DictionaryValue* denial_dict = dict_update.Get();
  EXPECT_TRUE(denial_dict);

  base::ListValue* list_value = nullptr;
  bool has_list = denial_dict->GetList(kTestLanguage, &list_value);
  EXPECT_FALSE(has_list);

  // Calling GetDenialTimes will force creation of a properly populated list.
  DenialTimeUpdate update(prefs_.get(), kTestLanguage, 2);
  base::ListValue* time_list = update.GetDenialTimes();
  EXPECT_TRUE(time_list);
  EXPECT_EQ(0U, time_list->GetSize());
}

// Test that an existing update time record (which is a double in a dict)
// is automatically migrated to a list of update times instead.
TEST_F(TranslatePrefTest, DenialTimeUpdate_Migrate) {
  translate_prefs_->ResetDenialState();
  DictionaryPrefUpdate dict_update(
      prefs_.get(), TranslatePrefs::kPrefTranslateLastDeniedTimeForLanguage);
  base::DictionaryValue* denial_dict = dict_update.Get();
  EXPECT_TRUE(denial_dict);
  denial_dict->SetDouble(kTestLanguage, two_days_ago_.ToJsTime());

  base::ListValue* list_value = nullptr;
  bool has_list = denial_dict->GetList(kTestLanguage, &list_value);
  EXPECT_FALSE(has_list);

  // Calling GetDenialTimes will force creation of a properly populated list.
  DenialTimeUpdate update(prefs_.get(), kTestLanguage, 2);
  base::ListValue* time_list = update.GetDenialTimes();
  EXPECT_TRUE(time_list);

  has_list = denial_dict->GetList(kTestLanguage, &list_value);
  EXPECT_TRUE(has_list);
  EXPECT_EQ(time_list, list_value);
  EXPECT_EQ(1U, time_list->GetSize());
  EXPECT_EQ(two_days_ago_, update.GetOldestDenialTime());
}

TEST_F(TranslatePrefTest, DenialTimeUpdate_SlidingWindow) {
  DenialTimeUpdate update(prefs_.get(), kTestLanguage, 4);

  update.AddDenialTime(now_ - base::TimeDelta::FromMinutes(5));
  EXPECT_EQ(update.GetOldestDenialTime(),
            now_ - base::TimeDelta::FromMinutes(5));

  update.AddDenialTime(now_ - base::TimeDelta::FromMinutes(4));
  EXPECT_EQ(update.GetOldestDenialTime(),
            now_ - base::TimeDelta::FromMinutes(5));

  update.AddDenialTime(now_ - base::TimeDelta::FromMinutes(3));
  EXPECT_EQ(update.GetOldestDenialTime(),
            now_ - base::TimeDelta::FromMinutes(5));

  update.AddDenialTime(now_ - base::TimeDelta::FromMinutes(2));
  EXPECT_EQ(update.GetOldestDenialTime(),
            now_ - base::TimeDelta::FromMinutes(4));

  update.AddDenialTime(now_);
  EXPECT_EQ(update.GetOldestDenialTime(),
            now_ - base::TimeDelta::FromMinutes(3));

  update.AddDenialTime(now_);
  EXPECT_EQ(update.GetOldestDenialTime(),
            now_ - base::TimeDelta::FromMinutes(2));
}

TEST_F(TranslatePrefTest, UpdateLanguageList) {
  // Test with basic set of languages (no country codes).
  std::vector<std::string> languages{"en", "ja"};
  translate_prefs_->UpdateLanguageList(languages);
  EXPECT_EQ("en,ja", prefs_->GetString(kAcceptLanguagesPref));

  // Test with languages that have country codes. Expect accepted languages both
  // with and without a country code. (See documentation for
  // ExpandLanguageCodes.)
  languages = {"en-US", "ja", "en-CA"};
  translate_prefs_->UpdateLanguageList(languages);
#if defined(OS_CHROMEOS)
  EXPECT_EQ("en-US,ja,en-CA", prefs_->GetString(kPreferredLanguagesPref));
#endif
  EXPECT_EQ("en-US,en,ja,en-CA", prefs_->GetString(kAcceptLanguagesPref));
}

}  // namespace translate
