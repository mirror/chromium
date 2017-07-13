// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/features/new_tab_factory.h"

#include "base/memory/singleton.h"
#include "chrome/browser/feature_engagement_tracker/feature_engagement_tracker_factory.h"
#include "chrome/browser/feature_engagement_tracker/features/new_tab_tracker.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace new_tab_help {

// static
NewTabFactory* NewTabFactory::GetInstance() {
  return base::Singleton<NewTabFactory>::get();
}

NewTabTracker* NewTabFactory::GetForProfile(Profile* profile) {
  return static_cast<NewTabTracker*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

NewTabFactory::NewTabFactory()
    : BrowserContextKeyedServiceFactory(
          "NewTabTracker",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(FeatureEngagementTrackerFactory::GetInstance());
}

NewTabFactory::~NewTabFactory() = default;

KeyedService* NewTabFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new NewTabTracker(Profile::FromBrowserContext(context));
}

content::BrowserContext* NewTabFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace new_tab_help
