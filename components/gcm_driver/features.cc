// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gcm_driver/features.h"
#include "base/metrics/field_trial_param_associator.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"

#include <algorithm>
#include <map>

namespace gcm {

namespace features {

const base::Feature kInvalidateTokenFeature{"TokenInvalidAfterDays",
                                            base::FEATURE_DISABLED_BY_DEFAULT};
base::TimeDelta GetTokenInvalidationInterval() {
  if (!base::FeatureList::IsEnabled(kInvalidateTokenFeature))
    return base::TimeDelta::FromDays(kDefaultTokenInvalidationPeriod);
  std::string override_value = base::GetFieldTrialParamValueByFeature(
      kInvalidateTokenFeature, kParamNameTokenInvalidationPeriodDays);

  if (!override_value.empty()) {
    int override_value_days;
    if (base::StringToInt(override_value, &override_value_days))
      return base::TimeDelta::FromDays(override_value_days);
  }
  return base::TimeDelta::FromDays(kDefaultTokenInvalidationPeriod);
}

}  // namespace features

}  // namespace gcm
