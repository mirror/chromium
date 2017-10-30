// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web_view/internal/language/web_view_url_language_histogram_factory.h"

namespace ios_web_view {

// static
WebViewUrlLanguageHistogramFactory*
WebViewUrlLanguageHistogramFactory::GetInstance() {
  return base::Singleton<WebViewUrlLanguageHistogramFactory>::get();
}

// static
language::UrlLanguageHistogram*
WebViewUrlLanguageHistogramFactory::GetForBrowserState(
    WebViewBrowserState* state) {
  return static_cast<language::UrlLanguageHistogram*>(
      GetInstance()->GetServiceForBrowserState(state, true));
}

WebViewUrlLanguageHistogramFactory::WebViewUrlLanguageHistogramFactory()
    : BrowserStateKeyedServiceFactory(
          "UrlLanguageHistogram",
          BrowserStateDependencyManager::GetInstance()) {}

WebViewUrlLanguageHistogramFactory::~WebViewUrlLanguageHistogramFactory() {}

std::unique_ptr<KeyedService>
WebViewUrlLanguageHistogramFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  WebViewBrowserState* web_view_browser_state =
      WebViewBrowserState::FromBrowserState(context);
  return base::MakeUnique<language::UrlLanguageHistogram>(
      web_view_browser_state->GetPrefs());
}

void WebViewUrlLanguageHistogramFactory::RegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* const registry) {
  language::UrlLanguageHistogram::RegisterProfilePrefs(registry);
}
                                                                                

}  // namespace ios_web_view
