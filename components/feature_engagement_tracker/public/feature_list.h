// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_FEATURE_LIST_H_
#define COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_FEATURE_LIST_H_

#include <vector>

#include "base/feature_list.h"
#include "components/flags_ui/feature_entry.h"

namespace feature_engagement_tracker {
using FeatureVector = std::vector<const base::Feature*>;

// The param name for the FeatureVariation configuration, which is used by
// chrome://flags to set the variable name for the selected feature. The
// FeatureEngagementTracker backend will then read this to figure out which
// feature (if any) was selected by the end user.
extern const char kIPHDemoModeFeatureChoiceParam[];

// The length of kIPHDemoModeChoiceVariations. This must be updated whenever
// new features are added to the demo mode selection.
const size_t kIPHDemoModeChoiceVariationsLen = 3;

// All variations to use with the demo mode. The size of this array must match
// the definition in feature_list.cc. This is used by the chrome://flags
// page to be able to list the different features that can be enabled on
// their own in demo mode.
extern const flags_ui::FeatureEntry::FeatureVariation
    kIPHDemoModeChoiceVariations[kIPHDemoModeChoiceVariationsLen];

// Returns all the features that are in use for engagement tracking.
FeatureVector GetAllFeatures();

}  // namespace feature_engagement_tracker

#endif  // COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_FEATURE_LIST_H_
