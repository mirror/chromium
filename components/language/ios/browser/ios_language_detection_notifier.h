// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LANUGUAGE_IOS_BROWSER_IOS_LANGUAGE_DETECTION_NOTIFIER_H_
#define COMPONENTS_LANUGUAGE_IOS_BROWSER_IOS_LANGUAGE_DETECTION_NOTIFIER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "ios/web/public/web_state/web_state_user_data.h"

namespace translate {
struct LanguageDetectionDetails;
}  // namespace translate

namespace language {

class UrlLanguageHistogram;

// Dispatches language detection messages to language and translate components.
class IOSLanguageDetectionNotifier
    : public web::WebStateUserData<IOSLanguageDetectionNotifier> {
 public:
  using Callback =
      base::RepeatingCallback<void(const translate::LanguageDetectionDetails&)>;

  ~IOSLanguageDetectionNotifier() override;

  // Attach a new notifier to the given WebState. We cannot use the
  // implementation from WebStateUserData as we are injecting the histogram and
  // translate callback differently on iOS and iOS WebView.
  static void CreateForWebState(web::WebState* web_state,
                                const Callback& translate_callback,
                                UrlLanguageHistogram* url_language_histogram);

  // Called on page language detection.
  void OnLanguageDetermined(const translate::LanguageDetectionDetails& details);

  // Add an extra listener for language detection. May only be used in tests.
  void SetExtraCallbackForTesting(const Callback& callback);

 private:
  IOSLanguageDetectionNotifier(
      const Callback& translate_callback,
      UrlLanguageHistogram* const url_language_histogram);
  friend class web::WebStateUserData<IOSLanguageDetectionNotifier>;

  const Callback translate_callback_;
  UrlLanguageHistogram* const url_language_histogram_;

  Callback extra_callback_for_testing_;

  DISALLOW_COPY_AND_ASSIGN(IOSLanguageDetectionNotifier);
};

}  // namespace language

#endif  // COMPONENTS_LANUGUAGE_IOS_BROWSER_IOS_LANGUAGE_DETECTION_NOTIFIER_H_
