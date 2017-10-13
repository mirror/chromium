// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LANUGUAGE_IOS_BROWSER_IOS_LANGUAGE_DETECTION_CLIENT_H_
#define COMPONENTS_LANUGUAGE_IOS_BROWSER_IOS_LANGUAGE_DETECTION_CLIENT_H_

#include "base/callback.h"
#include "base/macros.h"
#include "components/language/ios/browser/ios_language_detection_driver.h"
#include "ios/web/public/web_state/web_state_user_data.h"

namespace translate {
struct LanguageDetectionDetails;
}  // namespace translate

namespace language {

// Dispatches language detection messages to language and translate components.
class IOSLanguageDetectionClient
    : public web::WebStateUserData<IOSLanguageDetectionClient> {
 public:
  ~IOSLanguageDetectionClient() override;

  // Called before first use to populate the embedder-specific driver.
  void set_driver(std::unique_ptr<IOSLanguageDetectionDriver> driver) {
    driver_ = std::move(driver);
  }

  // Used to directly notify test code of language detection.
  void set_callback_for_testing(
      base::RepeatingCallback<void(const translate::LanguageDetectionDetails&)>
          callback_for_testing) {
    callback_for_testing_ = std::move(callback_for_testing);
  }

  // Called on page language detection. Virtual for testing.
  virtual void OnLanguageDetermined(
      const translate::LanguageDetectionDetails& details);

 protected:
  // web::WebStateUserData contract.
  explicit IOSLanguageDetectionClient(web::WebState* web_state);
  friend class web::WebStateUserData<IOSLanguageDetectionClient>;

 private:
  std::unique_ptr<IOSLanguageDetectionDriver> driver_;
  base::RepeatingCallback<void(const translate::LanguageDetectionDetails&)>
      callback_for_testing_;

  DISALLOW_COPY_AND_ASSIGN(IOSLanguageDetectionClient);
};

}  // namespace language

#endif  // COMPONENTS_LANUGUAGE_IOS_BROWSER_IOS_LANGUAGE_DETECTION_CLIENT_H_
