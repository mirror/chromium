// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LANGUAGE_IOS_BROWSER_LANGUAGE_DETECTION_CONTROLLER_H_
#define COMPONENTS_LANGUAGE_IOS_BROWSER_LANGUAGE_DETECTION_CONTROLLER_H_

#include <memory>
#include <string>

#include "base/callback_list.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "ios/web/public/web_state/web_state_observer.h"
#include "ios/web/public/web_state/web_state_user_data.h"

class GURL;
@class JsLanguageDetectionManager;

namespace base {
class DictionaryValue;
}

namespace net {
class HttpResponseHeaders;
}

namespace web {
class NavigationContext;
class WebState;
}  // namespace web

namespace language {

class LanguageDetectionController
    : public web::WebStateObserver,
      public web::WebStateUserData<LanguageDetectionController> {
 public:
  ~LanguageDetectionController() override;

 protected:
  // WebStateUserData contract.
  LanguageDetectionController(
      web::WebState* web_state,
      JsLanguageDetectionManager* js_for_testing = nullptr);
  friend class web::WebStateUserData<LanguageDetectionController>;

 private:
  // Test access to private methods.
  friend class LanguageDetectionControllerTest;
  FRIEND_TEST_ALL_PREFIXES(LanguageDetectionControllerTest, OnTextCaptured);
  FRIEND_TEST_ALL_PREFIXES(LanguageDetectionControllerTest,
                           MissingHttpContentLanguage);

  // Starts the page language detection and initiates the translation process.
  void StartLanguageDetection();

  // Handles the "languageDetection.textCaptured" javascript command.
  // |interacting| is true if the user is currently interacting with the page.
  bool OnTextCaptured(const base::DictionaryValue& value,
                      const GURL& url,
                      bool interacting);

  // Completion handler used to retrieve the text buffered by the
  // JsLanguageDetectionManager.
  void OnTextRetrieved(const std::string& http_content_language,
                       const std::string& html_lang,
                       const GURL& url,
                       const base::string16& text);

  // Extracts "content-language" header into content_language_header_ variable.
  void ExtractContentLanguageHeader(net::HttpResponseHeaders* headers);

  // web::WebStateObserver implementation:
  void PageLoaded(
      web::PageLoadCompletionStatus load_completion_status) override;
  void DidFinishNavigation(web::NavigationContext* navigation_context) override;
  void WebStateDestroyed() override;

  JsLanguageDetectionManager* js_manager_;
  std::string content_language_header_;
  base::WeakPtrFactory<LanguageDetectionController> weak_method_factory_;

  DISALLOW_COPY_AND_ASSIGN(LanguageDetectionController);
};

}  // namespace language

#endif  // COMPONENTS_LANGUAGE_IOS_BROWSER_LANGUAGE_DETECTION_CONTROLLER_H_
