// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_LANGUAGE_WEB_VIEW_LANGUAGE_DETECTION_DRIVER_H_
#define IOS_WEB_VIEW_INTERNAL_LANGUAGE_WEB_VIEW_LANGUAGE_DETECTION_DRIVER_H_

#include "base/macros.h"
#include "components/language/ios/browser/ios_language_detection_driver.h"
#include "ios/web/public/web_state/web_state_observer.h"

namespace ios_web_view {

class WebViewLanguageDetectionDriver
    : public language::IOSLanguageDetectionDriver,
      public web::WebStateObserver {
 public:
  explicit WebViewLanguageDetectionDriver(web::WebState* web_state);

  language::UrlLanguageHistogram* GetUrlLanguageHistogram() override;
  translate::IOSTranslateDriver* GetTranslateDriver() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebViewLanguageDetectionDriver);
};

void CreateWebViewLanguageDetectionClient(web::WebState* web_state);

}  // namespace ios_web_view

#endif  // IOS_WEB_VIEW_INTERNAL_LANGUAGE_WEB_VIEW_LANGUAGE_DETECTION_DRIVER_H_
