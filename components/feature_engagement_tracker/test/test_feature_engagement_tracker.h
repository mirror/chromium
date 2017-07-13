// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_TEST_TEST_FEATURE_ENGAGEMENT_TRACKER_H_
#define COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_TEST_TEST_FEATURE_ENGAGEMENT_TRACKER_H_

#include "components/feature_engagement_tracker/public/feature_engagement_tracker.h"

namespace feature_engagement_tracker {

// The TestFeatureEngagementTracker can be used as a testing mode
// Feature Engagement Tracker.
class TestFeatureEngagementTracker : public FeatureEngagementTracker {
 public:
  static std::unique_ptr<FeatureEngagementTracker> Create();

 protected:
  TestFeatureEngagementTracker();
  ~TestFeatureEngagementTracker() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestFeatureEngagementTracker);
};

}  // namespace feature_engagement_tracker

#endif  // COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_TEST_TEST_FEATURE_ENGAGEMENT_TRACKER_H_
