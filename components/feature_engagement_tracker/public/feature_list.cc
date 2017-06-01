/// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement_tracker/public/feature_list.h"

#include "base/feature_list.h"
#include "components/feature_engagement_tracker/public/feature_constants.h"
#include "components/flags_ui/feature_entry.h"

namespace feature_engagement_tracker {

namespace {

#define VARIATION_PARAM(base_feature)                                      \
  const flags_ui::FeatureEntry::FeatureParam base_feature##Variation[] = { \
      {kIPHDemoModeFeatureChoiceParam, base_feature.name}}

#define VARIATION_ENTRY(base_feature)                                \
  {                                                                  \
    base_feature##Variation[0].param_value, base_feature##Variation, \
        arraysize(base_feature##Variation), nullptr                  \
  }

const base::Feature* kAllFeatures[] = {
    &kIPHDataSaverPreview, &kIPHDownloadPageFeature, &kIPHDownloadHomeFeature};

VARIATION_PARAM(kIPHDataSaverPreview);
VARIATION_PARAM(kIPHDownloadPageFeature);
VARIATION_PARAM(kIPHDownloadHomeFeature);

}  // namespace

const char kIPHDemoModeFeatureChoiceParam[] = "chosen_feature";

const flags_ui::FeatureEntry::FeatureVariation kIPHDemoModeChoiceVariations[] =
    {VARIATION_ENTRY(kIPHDataSaverPreview),
     VARIATION_ENTRY(kIPHDownloadPageFeature),
     VARIATION_ENTRY(kIPHDownloadHomeFeature)};

std::vector<const base::Feature*> GetAllFeatures() {
  return std::vector<const base::Feature*>(
      kAllFeatures, kAllFeatures + arraysize(kAllFeatures));
}

}  // namespace feature_engagement_tracker
