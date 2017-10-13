// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/ios/browser/ios_language_detection_client.h"

#include "components/language/core/browser/url_language_histogram.h"
#include "components/translate/core/common/language_detection_details.h"
#include "components/translate/ios/browser/ios_translate_driver.h"

DEFINE_WEB_STATE_USER_DATA_KEY(language::IOSLanguageDetectionClient);

namespace language {

IOSLanguageDetectionClient::IOSLanguageDetectionClient(
    web::WebState* const web_state)
    : driver_(nullptr), callback_for_testing_() {}

IOSLanguageDetectionClient::~IOSLanguageDetectionClient() = default;

void IOSLanguageDetectionClient::OnLanguageDetermined(
    const translate::LanguageDetectionDetails& details) {
  if (!driver_) {
    NOTREACHED();
    return;
  }

  // Update language histogram.
  UrlLanguageHistogram* const url_language_histogram =
      driver_->GetUrlLanguageHistogram();
  if (url_language_histogram && details.is_cld_reliable) {
    url_language_histogram->OnPageVisited(details.cld_language);
  }

  // Update translate driver.
  translate::IOSTranslateDriver* const translate_driver =
      driver_->GetTranslateDriver();
  if (translate_driver) {
    translate_driver->OnLanguageDetermined(details);
  }

  // Inform tests of language detection.
  if (!callback_for_testing_.is_null()) {
    std::move(callback_for_testing_).Run(details);
  }
}

}  // namespace language
