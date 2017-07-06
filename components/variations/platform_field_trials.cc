// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/platform_field_trials.h"

#include "base/macros.h"
#include "build/build_config.h"

namespace variations {

PlatformFieldTrials::PlatformFieldTrials() {}

PlatformFieldTrials::~PlatformFieldTrials() {}

void PlatformFieldTrials::SetupFieldTrials() {
  // TODO(kmilka): implement functionality for WebView
  return;
}

// Create field trials that will control feature list features. This should be
// called during the same timing window as
// FeatureList::AssociateReportingFieldTrial. |has_seed| indicates that the
// variations service used a seed to create field trials. This can be used to
// prevent associating a field trial with a feature that you expect to be
// controlled by the variations seed.
void PlatformFieldTrials::SetupFeatureControllingFieldTrials(
    bool has_seed,
    base::FeatureList* feature_list) {
  // TODO(kmilka): implement functionality for WebView
  return;
}

}  // namespace variations
