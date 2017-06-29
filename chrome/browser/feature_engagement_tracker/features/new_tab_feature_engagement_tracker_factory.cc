// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/features/new_tab_feature_engagement_tracker_factory.h"

#include "base/files/file_path.h"
#include "base/memory/singleton.h"
#include "chrome/browser/feature_engagement_tracker/features/new_tab_feature_engagement_tracker.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

// static
NewTabFeatureEngagementTrackerFactory*
NewTabFeatureEngagementTrackerFactory::GetInstance() {
  return base::Singleton<NewTabFeatureEngagementTrackerFactory>::get();
}

// static
new_tab_feature_engagement_tracker::NewTabFeatureEngagementTracker*
NewTabFeatureEngagementTrackerFactory::GetForProfile(Profile* profile) {
  return static_cast<
      new_tab_feature_engagement_tracker::NewTabFeatureEngagementTracker*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

NewTabFeatureEngagementTrackerFactory::NewTabFeatureEngagementTrackerFactory()
    : BrowserContextKeyedServiceFactory(
          "new_tab_feature_engagement_tracker::NewTabFeatureEngagementTracker",
          BrowserContextDependencyManager::GetInstance()) {}

NewTabFeatureEngagementTrackerFactory::
    ~NewTabFeatureEngagementTrackerFactory() {}

KeyedService* NewTabFeatureEngagementTrackerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  new_tab_feature_engagement_tracker::NewTabFeatureEngagementTracker* tracker =
      new new_tab_feature_engagement_tracker::NewTabFeatureEngagementTracker(
          profile);
  return tracker;
}

content::BrowserContext*
NewTabFeatureEngagementTrackerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
