// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/net/variations_cmd_helper.h"

#include <stddef.h>

#include "base/feature_list.cc"
#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "components/variations/field_trial_config/field_trial_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace variations {

TEST(VariationsCmdHelperTest, GetVariationsCmd) {
  std::string trial_list = "trial1/group1/*trial2/group2/";
  std::string param_list = "trial1.group1:p1/v1/p2/2";
  std::string enable_feature_list = "feature1<trial1";
  std::string disable_feature_list = "feature2<trial2";

  base::FieldTrialList field_trial_list(nullptr);
  AssociateParamsFromString(param_list);
  base::FieldTrialList::CreateTrialsFromString(trial_list,
                                               std::set<std::string>());
  base::FeatureList::FeatureList::GetInstance()->InitializeFromCommandLine(
      enable_feature_list, disable_feature_list);

  std::string output;
  GetVariationsCmd(&output);
  EXPECT_NE(output.find(trial_list), std::string::npos);
  EXPECT_NE(output.find(param_list), std::string::npos);
  EXPECT_NE(output.find(enable_feature_list), std::string::npos);
  EXPECT_NE(output.find(disable_feature_list), std::string::npos);
}
}  // namespace variations