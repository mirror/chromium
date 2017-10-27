// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/ios/browser/ios_language_detection_client.h"

#include "components/language/core/browser/url_language_histogram.h"
#include "components/translate/core/common/language_detection_details.h"

DEFINE_WEB_STATE_USER_DATA_KEY(language::IOSLanguageDetectionClient);

namespace language {

IOSLanguageDetectionClient::IOSLanguageDetectionClient(
    const Callback& translate_callback,
    UrlLanguageHistogram* const url_language_histogram)
    : translate_callback_(translate_callback),
      url_language_histogram_(url_language_histogram) {}

IOSLanguageDetectionClient::~IOSLanguageDetectionClient() = default;

// static
void IOSLanguageDetectionClient::CreateForWebState(
    web::WebState* web_state,
    const Callback& translate_callback,
    UrlLanguageHistogram* const url_language_histogram) {
  DCHECK(web_state);
  if (!FromWebState(web_state)) {
    web_state->SetUserData(UserDataKey(),
                           base::WrapUnique(new IOSLanguageDetectionClient(
                               translate_callback, url_language_histogram)));
  }
}

void IOSLanguageDetectionClient::OnLanguageDetermined(
    const translate::LanguageDetectionDetails& details) {
  // Update language histogram.
  if (url_language_histogram_ && details.is_cld_reliable) {
    url_language_histogram_->OnPageVisited(details.cld_language);
  }

  // Update translate.
  translate_callback_.Run(details);

  // Optionally update testing callback.
  if (extra_callback_for_testing_) {
    extra_callback_for_testing_.Run(details);
  }
}

void IOSLanguageDetectionClient::SetExtraCallbackForTesting(
    const Callback& callback) {
  extra_callback_for_testing_ = callback;
}

}  // namespace language
