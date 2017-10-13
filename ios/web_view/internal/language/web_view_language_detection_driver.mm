// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web_view/internal/language/web_view_language_detection_driver.h"

#include "base/memory/ptr_util.h"
#include "components/language/ios/browser/ios_language_detection_client.h"
#include "components/translate/ios/browser/ios_translate_driver.h"
#include "ios/web_view/internal/translate/web_view_translate_client.h"

namespace ios_web_view {

WebViewLanguageDetectionDriver::WebViewLanguageDetectionDriver(
    web::WebState* const web_state)
    : web::WebStateObserver(web_state) {}

translate::IOSTranslateDriver*
WebViewLanguageDetectionDriver::GetTranslateDriver() {
  return WebViewTranslateClient::FromWebState(web_state())
      ->GetTranslateDriver();
}

// We currently don't maintain a URL language histogram for iOS web view.
// TODO(crbug.com/729859): update this when the histogram becomes available.
language::UrlLanguageHistogram*
WebViewLanguageDetectionDriver::GetUrlLanguageHistogram() {
  return nullptr;
}

void CreateWebViewLanguageDetectionClient(web::WebState* const web_state) {
  language::IOSLanguageDetectionClient::CreateForWebState(web_state);
  language::IOSLanguageDetectionClient::FromWebState(web_state)->set_driver(
      base::MakeUnique<WebViewLanguageDetectionDriver>(web_state));
}

}  // namespace ios_web_view
