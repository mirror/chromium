// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_FEATURES_NEW_TAB_FEATURE_ENGAGEMENT_TRACKER_FACTORY_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_FEATURES_NEW_TAB_FEATURE_ENGAGEMENT_TRACKER_FACTORY_H_

#include "base/macros.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace new_tab_feature_engagement_tracker {
class NewTabFeatureEngagementTracker;
}  // namespace new_tab_feature_engagement_tracker

// NewTabFeatureEngagementTrackerFactory is the main client class for
// interaction with the new_tab_feature_engagement_tracker component.
class NewTabFeatureEngagementTrackerFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  // Returns singleton instance of FeatureEngagementTrackerFactory.
  static NewTabFeatureEngagementTrackerFactory* GetInstance();

  // Returns the FeatureEngagementTracker associated with the profile.
  static new_tab_feature_engagement_tracker::NewTabFeatureEngagementTracker*
  GetForProfile(Profile* profile);

 private:
  friend struct base::DefaultSingletonTraits<
      NewTabFeatureEngagementTrackerFactory>;

  NewTabFeatureEngagementTrackerFactory();
  ~NewTabFeatureEngagementTrackerFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(NewTabFeatureEngagementTrackerFactory);
};

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_FEATURES_NEW_TAB_FEATURE_ENGAGEMENT_TRACKER_FACTORY_H_
