// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/core/browser/language_detector.h"

#include "base/memory/weak_ptr.h"
#include "components/translate/core/common/language_detection_details.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace translate {

// Compare LanguageDetectionDetails on adopted language.
bool operator==(const LanguageDetectionDetails& lhs,
                const LanguageDetectionDetails& rhs) {
  return lhs.adopted_language == rhs.adopted_language;
}

}  // namespace translate

namespace language {

// Observer that logs detection details to the given vector.
class MockObserver : public language::LanguageDetector::Observer,
                     public base::SupportsWeakPtr<MockObserver> {
 public:
  MockObserver(
      std::vector<translate::LanguageDetectionDetails>* const received_details)
      : received_details_(received_details) {}

  void OnLanguageDetected(
      const translate::LanguageDetectionDetails& details) override {
    received_details_->push_back(details);
  }

  std::vector<translate::LanguageDetectionDetails>* const received_details_;
};

// Test that the callbacks are sent correctly.
TEST(LanguageDetectorTest, CallbacksSent) {
  LanguageDetector lang_detector;

  std::vector<translate::LanguageDetectionDetails> v1, v2;
  MockObserver ob1(&v1), ob2(&v2);

  translate::LanguageDetectionDetails d1, d2, d3;
  d1.adopted_language = "en";
  d2.adopted_language = "de";
  d3.adopted_language = "fr";

  lang_detector.NotifyLanguageDetectionObservers(d1);
  lang_detector.AddLanguageDetectionObserver(ob1.AsWeakPtr());
  lang_detector.NotifyLanguageDetectionObservers(d2);
  lang_detector.AddLanguageDetectionObserver(ob2.AsWeakPtr());
  lang_detector.NotifyLanguageDetectionObservers(d3);

  EXPECT_THAT(v1, testing::ElementsAre(d2, d3));
  EXPECT_THAT(v2, testing::ElementsAre(d3));
}

// Test that an observer falling out of scope is handled gracefully.
TEST(LanguageDetectorTest, InvalidationHandled) {
  LanguageDetector lang_detector;

  std::vector<translate::LanguageDetectionDetails> v1, v2;
  {
    // Let the observer die before the notification is sent.
    MockObserver ob1(&v1);
    lang_detector.AddLanguageDetectionObserver(ob1.AsWeakPtr());
  }

  // Keep this observer around.
  MockObserver ob2(&v2);
  lang_detector.AddLanguageDetectionObserver(ob2.AsWeakPtr());

  translate::LanguageDetectionDetails details;
  details.adopted_language = "en";
  lang_detector.NotifyLanguageDetectionObservers(details);

  EXPECT_THAT(v1, testing::IsEmpty());
  EXPECT_THAT(v2, testing::ElementsAre(details));
}

}  // namespace language
