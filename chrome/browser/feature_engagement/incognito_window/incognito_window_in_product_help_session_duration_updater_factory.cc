// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/incognito_window/incognito_window_in_product_help_session_duration_updater_factory.h"

#include "base/memory/singleton.h"
#include "chrome/browser/feature_engagement/incognito_window/incognito_window_in_product_help_session_duration_updater.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace feature_engagement {

// static
IncognitoWindowInProductHelpSessionDurationUpdaterFactory*
IncognitoWindowInProductHelpSessionDurationUpdaterFactory::GetInstance() {
  return base::Singleton<
      IncognitoWindowInProductHelpSessionDurationUpdaterFactory>::get();
}

IncognitoWindowInProductHelpSessionDurationUpdater*
IncognitoWindowInProductHelpSessionDurationUpdaterFactory::GetForProfile(
    Profile* profile) {
  return static_cast<IncognitoWindowInProductHelpSessionDurationUpdater*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

IncognitoWindowInProductHelpSessionDurationUpdaterFactory::
    IncognitoWindowInProductHelpSessionDurationUpdaterFactory()
    : BrowserContextKeyedServiceFactory(
          "IncognitoWindowInProductHelpSessionDurationUpdater",
          BrowserContextDependencyManager::GetInstance()) {}

IncognitoWindowInProductHelpSessionDurationUpdaterFactory::
    ~IncognitoWindowInProductHelpSessionDurationUpdaterFactory() = default;

KeyedService* IncognitoWindowInProductHelpSessionDurationUpdaterFactory::
    BuildServiceInstanceFor(content::BrowserContext* context) const {
  return new IncognitoWindowInProductHelpSessionDurationUpdater(
      Profile::FromBrowserContext(context)->GetPrefs());
}

content::BrowserContext*
IncognitoWindowInProductHelpSessionDurationUpdaterFactory::
    GetBrowserContextToUse(content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool IncognitoWindowInProductHelpSessionDurationUpdaterFactory::
    ServiceIsCreatedWithBrowserContext() const {
  // Start IncognitoWindowInProductHelpSessionDurationUpdater early so the
  // session time starts tracking.
  return true;
}

}  // namespace feature_engagement
