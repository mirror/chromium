// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_TEST_TEST_FEATURE_ENGAGEMENT_TRACKER_H_
#define COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_TEST_TEST_FEATURE_ENGAGEMENT_TRACKER_H_

#include "components/feature_engagement_tracker/public/feature_engagement_tracker.h"

namespace feature_engagement_tracker {

// Provides a test FeatureEngagementTracker that makes all non-relevant
// conditions true so you can test per-feature specific configurations.
// Note: Your feature config params must have availability set to "any",
// or the FeatureConfigConditionValidator will return false.
std::unique_ptr<FeatureEngagementTracker> CreateTestFeatureEngagementTracker();

}  // namespace feature_engagement_tracker

#endif  // COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_TEST_TEST_FEATURE_ENGAGEMENT_TRACKER_H_
