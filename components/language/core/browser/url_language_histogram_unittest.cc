// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/core/browser/url_language_histogram.h"

#include "components/language/core/browser/language_detector.h"
#include "components/prefs/testing_pref_service.h"
#include "components/translate/core/common/language_detection_details.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;
using testing::FloatEq;
using testing::Gt;
using testing::SizeIs;

namespace {

const char kLang1[] = "en";
const char kLang2[] = "de";
const char kLang3[] = "es";
const char kLang4[] = "fr";

translate::LanguageDetectionDetails DetailsFromCld(const char* const lang,
                                                   const bool reliable) {
  translate::LanguageDetectionDetails details;
  details.cld_language = lang;
  details.is_cld_reliable = reliable;
  return details;
}

}  // namespace

namespace language {

bool operator==(const UrlLanguageHistogram::LanguageInfo& lhs,
                const UrlLanguageHistogram::LanguageInfo& rhs) {
  return lhs.language_code == rhs.language_code;
}

TEST(UrlLanguageHistogramTest, ListSorted) {
  TestingPrefServiceSimple prefs;
  UrlLanguageHistogram::RegisterProfilePrefs(prefs.registry());
  UrlLanguageHistogram hist(&prefs);

  for (int i = 0; i < 50; i++) {
    hist.OnLanguageDetected(DetailsFromCld(kLang1, true));
    hist.OnLanguageDetected(DetailsFromCld(kLang1, true));
    hist.OnLanguageDetected(DetailsFromCld(kLang1, true));
    hist.OnLanguageDetected(DetailsFromCld(kLang2, true));
  }

  // Note: LanguageInfo's operator== only checks the language code, not the
  // frequency.
  EXPECT_THAT(hist.GetTopLanguages(),
              ElementsAre(UrlLanguageHistogram::LanguageInfo(kLang1, 0.0f),
                          UrlLanguageHistogram::LanguageInfo(kLang2, 0.0f)));
}

TEST(UrlLanguageHistogramTest, ListSortedReversed) {
  TestingPrefServiceSimple prefs;
  UrlLanguageHistogram::RegisterProfilePrefs(prefs.registry());
  UrlLanguageHistogram hist(&prefs);

  for (int i = 0; i < 50; i++) {
    hist.OnLanguageDetected(DetailsFromCld(kLang2, true));
    hist.OnLanguageDetected(DetailsFromCld(kLang1, true));
    hist.OnLanguageDetected(DetailsFromCld(kLang1, true));
    hist.OnLanguageDetected(DetailsFromCld(kLang1, true));
  }

  // Note: LanguageInfo's operator== only checks the language code, not the
  // frequency.
  EXPECT_THAT(hist.GetTopLanguages(),
              ElementsAre(UrlLanguageHistogram::LanguageInfo(kLang1, 0.0f),
                          UrlLanguageHistogram::LanguageInfo(kLang2, 0.0f)));
}

TEST(UrlLanguageHistogramTest, CldLanguageRecorded) {
  TestingPrefServiceSimple prefs;
  UrlLanguageHistogram::RegisterProfilePrefs(prefs.registry());
  UrlLanguageHistogram hist(&prefs);

  // CLD is lang 1, all other fields are lang 3.
  translate::LanguageDetectionDetails details1 = DetailsFromCld(kLang1, true);
  details1.content_language = details1.html_root_language =
      details1.adopted_language = kLang3;

  // CLD is lang 2, all other fields are lang 4.
  translate::LanguageDetectionDetails details2 = DetailsFromCld(kLang2, true);
  details2.content_language = details2.html_root_language =
      details2.adopted_language = kLang4;

  for (int i = 0; i < 50; i++) {
    hist.OnLanguageDetected(details1);
    hist.OnLanguageDetected(details1);
    hist.OnLanguageDetected(details1);
    hist.OnLanguageDetected(details2);
  }

  // Check that only the CLD language is recorded.
  // Note: LanguageInfo's operator== only checks the language code, not the
  // frequency.
  EXPECT_THAT(hist.GetTopLanguages(),
              ElementsAre(UrlLanguageHistogram::LanguageInfo(kLang1, 0.0f),
                          UrlLanguageHistogram::LanguageInfo(kLang2, 0.0f)));
}

TEST(UrlLanguageHistogramTest, RightFrequencies) {
  TestingPrefServiceSimple prefs;
  UrlLanguageHistogram::RegisterProfilePrefs(prefs.registry());
  UrlLanguageHistogram hist(&prefs);

  for (int i = 0; i < 50; i++) {
    hist.OnLanguageDetected(DetailsFromCld(kLang1, true));
    hist.OnLanguageDetected(DetailsFromCld(kLang1, true));
    hist.OnLanguageDetected(DetailsFromCld(kLang1, true));
    hist.OnLanguageDetected(DetailsFromCld(kLang2, true));

    // Add an unreliable language detection (which should be ignored).
    hist.OnLanguageDetected(DetailsFromCld(kLang2, false));
  }

  // Corresponding frequencies are given by the hist.
  EXPECT_THAT(hist.GetLanguageFrequency(kLang1), FloatEq(0.75f));
  EXPECT_THAT(hist.GetLanguageFrequency(kLang2), FloatEq(0.25f));
  // An unknown language gets frequency 0.
  EXPECT_THAT(hist.GetLanguageFrequency(kLang3), 0);
  // Unreliable detections are not recorded.
  EXPECT_THAT(hist.GetLanguageFrequency(kLang4), 0);
}

TEST(UrlLanguageHistogramTest, RareLanguageDiscarded) {
  TestingPrefServiceSimple prefs;
  UrlLanguageHistogram::RegisterProfilePrefs(prefs.registry());
  UrlLanguageHistogram hist(&prefs);

  hist.OnLanguageDetected(DetailsFromCld(kLang2, true));

  for (int i = 0; i < 900; i++)
    hist.OnLanguageDetected(DetailsFromCld(kLang1, true));

  // Lang 2 is in the hist.
  EXPECT_THAT(hist.GetLanguageFrequency(kLang2), Gt(0.0f));

  // Another 100 visits cause the cleanup (total > 1000).
  for (int i = 0; i < 100; i++)
    hist.OnLanguageDetected(DetailsFromCld(kLang1, true));
  // Lang 2 is removed from the hist.
  EXPECT_THAT(hist.GetTopLanguages(),
              ElementsAre(UrlLanguageHistogram::LanguageInfo{kLang1, 1}));
}

TEST(UrlLanguageHistogramTest, ShouldClearHistoryIfAllTimes) {
  TestingPrefServiceSimple prefs;
  UrlLanguageHistogram::RegisterProfilePrefs(prefs.registry());
  UrlLanguageHistogram hist(&prefs);

  for (int i = 0; i < 100; i++) {
    hist.OnLanguageDetected(DetailsFromCld(kLang1, true));
  }

  EXPECT_THAT(hist.GetTopLanguages(), SizeIs(1));
  EXPECT_THAT(hist.GetLanguageFrequency(kLang1), FloatEq(1.0));

  hist.ClearHistory(base::Time(), base::Time::Max());

  EXPECT_THAT(hist.GetTopLanguages(), SizeIs(0));
  EXPECT_THAT(hist.GetLanguageFrequency(kLang1), FloatEq(0.0));
}

TEST(UrlLanguageHistogramTest, ShouldNotClearHistoryIfNotAllTimes) {
  TestingPrefServiceSimple prefs;
  UrlLanguageHistogram::RegisterProfilePrefs(prefs.registry());
  UrlLanguageHistogram hist(&prefs);

  for (int i = 0; i < 100; i++) {
    hist.OnLanguageDetected(DetailsFromCld(kLang1, true));
  }

  EXPECT_THAT(hist.GetTopLanguages(), SizeIs(1));
  EXPECT_THAT(hist.GetLanguageFrequency(kLang1), FloatEq(1.0));

  // Clearing only the last hour of the history has no effect.
  hist.ClearHistory(base::Time::Now() - base::TimeDelta::FromHours(2),
                    base::Time::Max());

  EXPECT_THAT(hist.GetTopLanguages(), SizeIs(1));
  EXPECT_THAT(hist.GetLanguageFrequency(kLang1), FloatEq(1.0));
}

TEST(UrlLanguageHistogramTest, LanguageDetectionCallbacks) {
  TestingPrefServiceSimple prefs;
  UrlLanguageHistogram::RegisterProfilePrefs(prefs.registry());
  UrlLanguageHistogram hist(&prefs);

  LanguageDetector ld1, ld2;
  hist.ObserveLanguageDetection(&ld1);
  hist.ObserveLanguageDetection(&ld2);

  for (int i = 0; i < 50; i++) {
    ld1.NotifyLanguageDetectionObservers(DetailsFromCld(kLang1, true));
    ld1.NotifyLanguageDetectionObservers(DetailsFromCld(kLang1, true));
    ld1.NotifyLanguageDetectionObservers(DetailsFromCld(kLang1, true));
    ld2.NotifyLanguageDetectionObservers(DetailsFromCld(kLang2, true));
  }

  // Note: LanguageInfo's operator== only checks the language code, not the
  // frequency.
  EXPECT_THAT(hist.GetTopLanguages(),
              ElementsAre(UrlLanguageHistogram::LanguageInfo(kLang1, 0.0f),
                          UrlLanguageHistogram::LanguageInfo(kLang2, 0.0f)));
}

}  // namespace language
