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
      int max_loaded_tabs_per_gb_ram,
      int min_loaded_tab_count,
      int max_loaded_tab_count,
      base::TimeDelta min_occluded_timeout,
      base::TimeDelta max_occluded_timeout) {
    EnableProactiveTabDiscarding();
    ProactiveTabDiscardParams params = {};
    GetProactiveTabDiscardParams(&params);
    EXPECT_EQ(max_loaded_tabs_per_gb_ram, params.max_loaded_tabs_per_gb_ram);
    EXPECT_EQ(min_loaded_tab_count, params.min_loaded_tab_count);
    EXPECT_EQ(max_loaded_tab_count, params.max_loaded_tab_count);
    EXPECT_EQ(min_occluded_timeout, params.min_occluded_timeout);
    EXPECT_EQ(max_occluded_timeout, params.max_occluded_timeout);
  }

  void ExpectDefaultProactiveTabDiscardParams() {
    ExpectProactiveTabDiscardingParams(
        kProactiveTabDiscard_MaxLoadedTabsPerGbRamDefault,
        kProactiveTabDiscard_MinLoadedTabCountDefault,
        kProactiveTabDiscard_MaxLoadedTabCountDefault,
        kProactiveTabDiscard_MinOccludedTimeoutDefault,
        kProactiveTabDiscard_MaxOccludedTimeoutDefault);
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
  SetParam(kProactiveTabDiscard_MaxLoadedTabsPerGbRamParam, "ab");
  SetParam(kProactiveTabDiscard_MinLoadedTabCountParam, "27.8");
  SetParam(kProactiveTabDiscard_MaxLoadedTabCountParam, "4e8");
  SetParam(kProactiveTabDiscard_MinOccludedTimeoutParam, "---");
  SetParam(kProactiveTabDiscard_MaxOccludedTimeoutParam, " ");
  ExpectDefaultProactiveTabDiscardParams();
}

TEST_F(TabManagerFeaturesTest, GetProactiveTabDiscardParams) {
  SetParam(kProactiveTabDiscard_MaxLoadedTabsPerGbRamParam, "7");
  SetParam(kProactiveTabDiscard_MinLoadedTabCountParam, "19");
  SetParam(kProactiveTabDiscard_MaxLoadedTabCountParam, "42");
  // These are expressed in seconds.
  SetParam(kProactiveTabDiscard_MinOccludedTimeoutParam, "60");
  SetParam(kProactiveTabDiscard_MaxOccludedTimeoutParam, "120");

  ExpectProactiveTabDiscardingParams(7, 19, 42,
                                     base::TimeDelta::FromSeconds(60),
                                     base::TimeDelta::FromSeconds(120));
}

}  // namespace resource_coordinator
