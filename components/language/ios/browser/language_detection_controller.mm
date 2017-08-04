// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/ios/browser/language_detection_controller.h"

#include <iostream>
#include <string>
#include <thread>

#include "base/bind.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "base/values.h"
#import "components/language/ios/browser/js_language_detection_manager.h"
#include "components/language/ios/browser/string_clipping_util.h"
#include "components/translate/core/common/language_detection_details.h"
#include "components/translate/core/language_detection/language_detection_util.h"
#import "ios/web/public/url_scheme_util.h"
#include "ios/web/public/web_state/js/crw_js_injection_receiver.h"
#import "ios/web/public/web_state/navigation_context.h"
#include "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(language::LanguageDetectionController);

namespace language {

namespace {
// Name for the UMA metric used to track text extraction time.
const char kTranslateCaptureText[] = "Translate.CaptureText";
// Prefix for the language detection javascript commands. Must be kept in sync
// with language_detection.js.
const char kCommandPrefix[] = "languageDetection";
}

LanguageDetectionController::LanguageDetectionController(
    web::WebState* web_state,
    JsLanguageDetectionManager* js_for_testing)
    : web::WebStateObserver(web_state), weak_method_factory_(this) {
  DCHECK(web::WebStateObserver::web_state());

  // Create the language detection manager.
  if (js_for_testing) {
    js_manager_ = js_for_testing;
  } else {
    CRWJSInjectionReceiver* receiver = web_state->GetJSInjectionReceiver();
    DCHECK(receiver);
    js_manager_ = static_cast<JsLanguageDetectionManager*>(
        [receiver instanceOfClass:[JsLanguageDetectionManager class]]);
    DCHECK(js_manager_);

    web_state->AddScriptCommandCallback(
        base::Bind(&LanguageDetectionController::OnTextCaptured,
                   base::Unretained(this)),
        kCommandPrefix);
  }
}

LanguageDetectionController::~LanguageDetectionController() {}

void LanguageDetectionController::StartLanguageDetection() {
  DCHECK(web_state());
  const GURL& url = web_state()->GetVisibleURL();
  if (!web::UrlHasWebScheme(url) || !web_state()->ContentIsHTML())
    return;
  [js_manager_ inject];
  [js_manager_ startLanguageDetection];
}

bool LanguageDetectionController::OnTextCaptured(
    const base::DictionaryValue& command,
    const GURL& url,
    bool interacting) {
  std::string textCapturedCommand;
  if (!command.GetString("command", &textCapturedCommand) ||
      textCapturedCommand != "languageDetection.textCaptured" ||
      !command.HasKey("translationAllowed")) {
    NOTREACHED();
    return false;
  }
  bool translation_allowed = false;
  command.GetBoolean("translationAllowed", &translation_allowed);
  if (!translation_allowed) {
    // Translation not allowed by the page. Done processing.
    return true;
  }
  if (!command.HasKey("captureTextTime") || !command.HasKey("htmlLang") ||
      !command.HasKey("httpContentLanguage")) {
    NOTREACHED();
    return false;
  }

  double capture_text_time = 0;
  command.GetDouble("captureTextTime", &capture_text_time);
  UMA_HISTOGRAM_TIMES(kTranslateCaptureText,
                      base::TimeDelta::FromMillisecondsD(capture_text_time));
  std::string html_lang;
  command.GetString("htmlLang", &html_lang);
  std::string http_content_language;
  command.GetString("httpContentLanguage", &http_content_language);
  // If there is no language defined in httpEquiv, use the HTTP header.
  if (http_content_language.empty())
    http_content_language = web_state()->GetContentLanguageHeader();

  [js_manager_ retrieveBufferedTextContent:
                   base::Bind(&LanguageDetectionController::OnTextRetrieved,
                              weak_method_factory_.GetWeakPtr(),
                              http_content_language, html_lang, url)];
  return true;
}

void LanguageDetectionController::OnTextRetrieved(
    const std::string& http_content_language,
    const std::string& html_lang,
    const GURL& url,
    const base::string16& text_content) {
  const base::string16 clipped_text = GetStringByClippingLastWord(
      text_content, language_detection::kMaxIndexChars);

  std::string cld_language;
  bool is_cld_reliable;
  std::string language = translate::DeterminePageLanguage(
      http_content_language, html_lang, clipped_text, &cld_language,
      &is_cld_reliable);
  if (language.empty())
    return;  // No language detected.

  translate::LanguageDetectionDetails details;
  details.time = base::Time::Now();
  details.url = url;
  details.content_language = http_content_language;
  details.cld_language = cld_language;
  details.is_cld_reliable = is_cld_reliable;
  details.html_root_language = html_lang;
  details.adopted_language = language;
  details.contents = clipped_text;

  NotifyLanguageDetectionObservers(details);
}

// web::WebStateObserver implementation:

void LanguageDetectionController::PageLoaded(
    web::PageLoadCompletionStatus load_completion_status) {
  if (load_completion_status == web::PageLoadCompletionStatus::SUCCESS)
    StartLanguageDetection();
}

void LanguageDetectionController::DidFinishNavigation(
    web::NavigationContext* navigation_context) {
  if (navigation_context->IsSameDocument())
    StartLanguageDetection();
}

void LanguageDetectionController::WebStateDestroyed() {
  web_state()->RemoveScriptCommandCallback(kCommandPrefix);
}

}  // namespace language
