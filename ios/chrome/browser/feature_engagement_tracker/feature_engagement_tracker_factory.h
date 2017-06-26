// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_FEATURE_ENGAGEMENT_TRACKER_FACTORY_H_
#define IOS_CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_FEATURE_ENGAGEMENT_TRACKER_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace feature_engagement_tracker {
class FeatureEngagementTracker;
}  // namespace feature_engagement_tracker

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace web {
class BrowserState;
}  // namespace web

namespace feature_engagement_tracker {
namespace ios {
class FeatureEngagementTrackerFactory : public BrowserStateKeyedServiceFactory {
 public:
  static FeatureEngagementTrackerFactory* GetInstance();

  static FeatureEngagementTracker* GetForBrowserState(web::BrowserState* state);

 protected:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;

 private:
  FeatureEngagementTrackerFactory();
  ~FeatureEngagementTrackerFactory() override;

  friend struct base::DefaultSingletonTraits<FeatureEngagementTrackerFactory>;

  DISALLOW_COPY_AND_ASSIGN(FeatureEngagementTrackerFactory);
};
}  // namespace ios
}  // namespace feature_engagement_tracker

#endif  // IOS_CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_FEATURE_ENGAGEMENT_TRACKER_FACTORY_H_
