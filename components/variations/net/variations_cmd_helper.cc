// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/net/variations_cmd_helper.h"

#include "base/base_switches.h"
#include "components/variations/active_field_trials.h"
#include "components/variations/variations_switches.h"
#include "net/base/escape.h"

namespace variations {

namespace {

// Format the provided |param_key| and |param_value| as commandline input.
std::string GenerateCmdParam(const std::string& param_key,
                             const std::string& param_value) {
  if (!param_value.empty())
    return " --" + param_key + "=\"" + param_value + "\"";

  return "";
}

}  // namespace

void GetVariationsCmd(std::string* output) {
  std::string field_trial_states;
  base::FieldTrialList::AllStatesToString(&field_trial_states, true);

  std::string field_trial_params;
  base::FieldTrialList::AllParamsToString(&field_trial_params, true,
                                          &net::EscapeQueryParamValue);

  std::string enable_features;
  std::string disable_features;
  base::FeatureList::GetInstance()->GetFeatureOverrides(&enable_features,
                                                        &disable_features);

  output->append(
      GenerateCmdParam(::switches::kForceFieldTrials, field_trial_states));
  output->append(
      GenerateCmdParam(switches::kForceFieldTrialParams, field_trial_params));
  output->append(
      GenerateCmdParam(::switches::kEnableFeatures, enable_features));
  output->append(
      GenerateCmdParam(::switches::kDisableFeatures, disable_features));
}

}  // namespace variations
