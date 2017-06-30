// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VARIATIONS_PLATFORM_FIELD_TRIALS_H_
#define COMPONENTS_VARIATIONS_PLATFORM_FIELD_TRIALS_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/variations/metrics.h"

namespace variations {

class PlatformFieldTrials {
 public:
  PlatformFieldTrials();
  ~PlatformFieldTrials();

  virtual void SetupFieldTrials();

  // Create field trials that will control feature list features. This should be
  // called during the same timing window as
  // FeatureList::AssociateReportingFieldTrial. |has_seed| indicates that the
  // variations service used a seed to create field trials. This can be used to
  // prevent associating a field trial with a feature that you expect to be
  // controlled by the variations seed.
  virtual void SetupFeatureControllingFieldTrials(
      bool has_seed,
      base::FeatureList* feature_list);
};

}  // namespace variations

#endif  // COMPONENTS_VARIATIONS_PLATFORM_FIELD_TRIALS_H_
