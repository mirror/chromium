// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LANUGUAGE_IOS_IOS_LANGUAGE_DETECTION_DRIVER_H_
#define COMPONENTS_LANUGUAGE_IOS_IOS_LANGUAGE_DETECTION_DRIVER_H_

namespace translate {
class IOSTranslateDriver;
}  // namespace translate

namespace language {

class UrlLanguageHistogram;

// Performs embedder-specific logic for language detection.
class IOSLanguageDetectionDriver {
 public:
  virtual ~IOSLanguageDetectionDriver() = default;

  virtual UrlLanguageHistogram* GetUrlLanguageHistogram() = 0;
  virtual translate::IOSTranslateDriver* GetTranslateDriver() = 0;
};

}  // namespace language

#endif  // COMPONENTS_LANUGUAGE_IOS_IOS_LANGUAGE_DETECTION_DRIVER_H_
