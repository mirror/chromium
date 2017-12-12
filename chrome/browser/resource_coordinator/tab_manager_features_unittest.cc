// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_manager_features.h"

#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/strings/string_piece.h"
#include "components/variations/variations_params_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace resource_coordinator {

namespace {

class TabManagerFeaturesTest : public testing::Test {
 public:
  // Enables the proactive tab discarding feature, and sets up the associated
  // variations parameter values.
  void EnableProactiveTabDiscarding() {
    std::set<std::string> features;
    features.insert(features::kProactiveTabDiscarding.name);
    variations_manager_.SetVariationParamsWithFeatureAssociations(
        "DummyTrial", params_, features);
  }

  void SetParam(const base::StringPiece& key, const base::StringPiece& value) {
    params_[key.as_string()] = value.as_string();
  }

  void ExpectProactiveTabDiscardingParams(
      int low_loaded_tab_count,
      int moderate_loaded_tab_count_per_gb,
      int excessive_loaded_tab_count,
      base::TimeDelta low_occluded_timeout,
      base::TimeDelta moderate_occluded_timeout,
      base::TimeDelta high_occluded_timeout) {
    EnableProactiveTabDiscarding();
    ProactiveTabDiscardParams params = {};
    GetProactiveTabDiscardParams(&params);
    EXPECT_EQ(low_loaded_tab_count, params.low_loaded_tab_count);
    EXPECT_EQ(moderate_loaded_tab_count_per_gb,
              params.moderate_loaded_tab_count_per_gb);
    EXPECT_EQ(excessive_loaded_tab_count, params.excessive_loaded_tab_count);
    EXPECT_EQ(low_occluded_timeout, params.low_occluded_timeout);
    EXPECT_EQ(moderate_occluded_timeout, params.moderate_occluded_timeout);
    EXPECT_EQ(high_occluded_timeout, params.high_occluded_timeout);
  }

  void ExpectDefaultProactiveTabDiscardParams() {
    ExpectProactiveTabDiscardingParams(
        kProactiveTabDiscard_LowLoadedTabCountDefault,
        kProactiveTabDiscard_ModerateLoadedTabsPerGbRamDefault,
        kProactiveTabDiscard_HighLoadedTabCountDefault,
        kProactiveTabDiscard_LowOccludedTimeoutDefault,
        kProactiveTabDiscard_ModerateOccludedTimeoutDefault,
        kProactiveTabDiscard_HighOccludedTimeoutDefault);
  }

 private:
  std::map<std::string, std::string> params_;
  variations::testing::VariationParamsManager variations_manager_;
};

}  // namespace

TEST_F(TabManagerFeaturesTest, GetProactiveTabDiscardParamsNoneGoesToDefault) {
  ExpectDefaultProactiveTabDiscardParams();
}

TEST_F(TabManagerFeaturesTest,
       GetProactiveTabDiscardParamsInvalidGoesToDefault) {
  SetParam(kProactiveTabDiscard_LowLoadedTabCountParam, "ab");
  SetParam(kProactiveTabDiscard_ModerateLoadedTabsPerGbRamParam, "27.8");
  SetParam(kProactiveTabDiscard_HighLoadedTabCountParam, "4e8");
  SetParam(kProactiveTabDiscard_LowOccludedTimeoutParam, "---");
  SetParam(kProactiveTabDiscard_ModerateOccludedTimeoutParam, " ");
  SetParam(kProactiveTabDiscard_HighOccludedTimeoutParam, "");
  ExpectDefaultProactiveTabDiscardParams();
}

TEST_F(TabManagerFeaturesTest, GetProactiveTabDiscardParams) {
  SetParam(kProactiveTabDiscard_MaxLoadedTabsPerGbRamParam, "7");
  SetParam(kProactiveTabDiscard_MinLoadedTabCountParam, "19");
  SetParam(kProactiveTabDiscard_MaxLoadedTabCountParam, "42");
  // These are expressed in seconds.
  SetParam(kProactiveTabDiscard_LowOccludedTimeoutParam, "60");
  SetParam(kProactiveTabDiscard_ModerateOccludedTimeoutParam, "120");
  SetParam(kProactiveTabDiscard_HighOccludedTimeoutParam, "247");

  ExpectProactiveTabDiscardingParams(
      7, 19, 42, base::TimeDelta::FromSeconds(60),
      base::TimeDelta::FromSeconds(120), base::TimeDelta::FromSeconds(247));
}

}  // namespace resource_coordinator
