// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/translate/language_model_factory.h"

#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/language/core/browser/url_language_histogram.h"

// static
LanguageModelFactory*
LanguageModelFactory::GetInstance() {
  return base::Singleton<LanguageModelFactory>::get();
}

// static
language::UrlLanguageHistogram* LanguageModelFactory::GetForBrowserContext(
    content::BrowserContext* browser_context) {
  return static_cast<language::UrlLanguageHistogram*>(
      GetInstance()->GetServiceForBrowserContext(browser_context, true));
}

LanguageModelFactory::LanguageModelFactory()
    : BrowserContextKeyedServiceFactory(
          "LanguageModel",
          BrowserContextDependencyManager::GetInstance()) {}

LanguageModelFactory::~LanguageModelFactory() {}

KeyedService* LanguageModelFactory::BuildServiceInstanceFor(
    content::BrowserContext* browser_context) const {
  Profile* profile = Profile::FromBrowserContext(browser_context);
  return new language::UrlLanguageHistogram(profile->GetPrefs());
}
