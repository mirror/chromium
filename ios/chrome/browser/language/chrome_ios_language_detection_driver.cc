// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/language/chrome_ios_language_detection_driver.h"

#include "base/memory/ptr_util.h"
#include "components/language/core/browser/url_language_histogram.h"
#include "components/language/ios/browser/ios_language_detection_client.h"
#include "components/translate/ios/browser/ios_translate_driver.h"
#include "ios/chrome/browser/language/url_language_histogram_factory.h"
#include "ios/chrome/browser/translate/chrome_ios_translate_client.h"

ChromeIOSLanguageDetectionDriver::ChromeIOSLanguageDetectionDriver(
    web::WebState* const web_state)
    : web::WebStateObserver(web_state) {}

language::UrlLanguageHistogram*
ChromeIOSLanguageDetectionDriver::GetUrlLanguageHistogram() {
  return UrlLanguageHistogramFactory::GetInstance()->GetForBrowserState(
      ChromeBrowserState::FromWebState(web_state()));
}

translate::IOSTranslateDriver*
ChromeIOSLanguageDetectionDriver::GetTranslateDriver() {
  return ChromeIOSTranslateClient::FromWebState(web_state())
      ->GetTranslateDriver();
}

void CreateChromeIOSLanguageDetectionClient(web::WebState* const web_state) {
  language::IOSLanguageDetectionClient::CreateForWebState(web_state);
  language::IOSLanguageDetectionClient::FromWebState(web_state)->set_driver(
      base::MakeUnique<ChromeIOSLanguageDetectionDriver>(web_state));
}
