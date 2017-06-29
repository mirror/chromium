// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/features/new_tab_feature_engagement_tracker_factory.h"

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/feature_engagement_tracker/features/new_tab_feature_engagement_tracker.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_constants.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

// static
NewTabFeatureEngagementTrackerFactory*
NewTabFeatureEngagementTrackerFactory::GetInstance() {
  return base::Singleton<NewTabFeatureEngagementTrackerFactory>::get();
}

// static
new_tab_feature_engagement_tracker::NewTabFeatureEngagementTracker*
NewTabFeatureEngagementTrackerFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<
      new_tab_feature_engagement_tracker::NewTabFeatureEngagementTracker*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

NewTabFeatureEngagementTrackerFactory::NewTabFeatureEngagementTrackerFactory()
    : BrowserContextKeyedServiceFactory(
          "new_tab_feature_engagement_tracker::NewTabFeatureEngagementTracker",
          BrowserContextDependencyManager::GetInstance()) {}

NewTabFeatureEngagementTrackerFactory::
    ~NewTabFeatureEngagementTrackerFactory() = default;

KeyedService* NewTabFeatureEngagementTrackerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);

  scoped_refptr<base::SequencedTaskRunner> background_task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND});

  base::FilePath storage_dir = profile->GetPath().Append(
      chrome::kNewTabFeatureEngagementTrackerStorageDirname);

  return feature_engagement_tracker::FeatureEngagementTracker::Create(
      storage_dir, background_task_runner);
}

content::BrowserContext*
NewTabFeatureEngagementTrackerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
