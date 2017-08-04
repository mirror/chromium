// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/core/browser/language_detector.h"

#include "components/translate/core/common/language_detection_details.h"

namespace language {

LanguageDetector::LanguageDetector() = default;

LanguageDetector::~LanguageDetector() = default;

void LanguageDetector::AddLanguageDetectionObserver(
    const base::WeakPtr<Observer> observer) {
  return language_detection_observers_.push_back(observer);
}

void LanguageDetector::NotifyLanguageDetectionObservers(
    const translate::LanguageDetectionDetails& details) {
  for (const base::WeakPtr<Observer>& observer :
       language_detection_observers_) {
    if (observer)
      observer->OnLanguageDetected(details);
  }
}

}  // namespace language
