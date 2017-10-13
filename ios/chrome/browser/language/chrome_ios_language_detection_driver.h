// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "components/language/ios/browser/ios_language_detection_driver.h"
#include "ios/web/public/web_state/web_state_observer.h"

class ChromeIOSLanguageDetectionDriver
    : public language::IOSLanguageDetectionDriver,
      public web::WebStateObserver {
 public:
  explicit ChromeIOSLanguageDetectionDriver(web::WebState* web_state);

  language::UrlLanguageHistogram* GetUrlLanguageHistogram() override;
  translate::IOSTranslateDriver* GetTranslateDriver() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeIOSLanguageDetectionDriver);
};

void CreateChromeIOSLanguageDetectionClient(web::WebState* web_state);
