// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/new_tab/new_tab_in_product_help_session_duration_updater_factory.h"

#include "base/memory/singleton.h"
#include "chrome/browser/feature_engagement/new_tab/new_tab_in_product_help_session_duration_updater.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace feature_engagement {

// static
NewTabInProductHelpSessionDurationUpdaterFactory*
NewTabInProductHelpSessionDurationUpdaterFactory::GetInstance() {
  return base::Singleton<
      NewTabInProductHelpSessionDurationUpdaterFactory>::get();
}

NewTabInProductHelpSessionDurationUpdater*
NewTabInProductHelpSessionDurationUpdaterFactory::GetForProfile(
    Profile* profile) {
  return static_cast<NewTabInProductHelpSessionDurationUpdater*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

NewTabInProductHelpSessionDurationUpdaterFactory::
    NewTabInProductHelpSessionDurationUpdaterFactory()
    : BrowserContextKeyedServiceFactory(
          "NewTabInProductHelpSessionDurationUpdater",
          BrowserContextDependencyManager::GetInstance()) {}

NewTabInProductHelpSessionDurationUpdaterFactory::
    ~NewTabInProductHelpSessionDurationUpdaterFactory() = default;

KeyedService*
NewTabInProductHelpSessionDurationUpdaterFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new NewTabInProductHelpSessionDurationUpdater(
      Profile::FromBrowserContext(context)->GetPrefs());
}

content::BrowserContext*
NewTabInProductHelpSessionDurationUpdaterFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool NewTabInProductHelpSessionDurationUpdaterFactory::
    ServiceIsCreatedWithBrowserContext() const {
  // Start NewTabInProductHelpSessionDurationUpdater early so the
  // session time starts tracking.
  return true;
}

}  // namespace feature_engagement
