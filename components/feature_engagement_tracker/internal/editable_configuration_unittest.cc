// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement_tracker/internal/editable_configuration.h"

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "base/test/scoped_feature_list.h"
#include "components/feature_engagement_tracker/internal/configuration.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace feature_engagement_tracker {

namespace {

const base::Feature kTestFeatureFoo{"test_foo",
                                    base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kTestFeatureBar{"test_bar",
                                    base::FEATURE_DISABLED_BY_DEFAULT};

class EditableConfigurationTest : public ::testing::Test {
 public:
  FeatureConfig GetValidFeatureConfig(std::string feature_used_event) {
    FeatureConfig feature_config;
    feature_config.valid = true;
    feature_config.feature_used_event = feature_used_event;
    return feature_config;
  }

  FeatureConfig GetInvalidFeatureConfig(std::string feature_used_event) {
    FeatureConfig feature_config;
    feature_config.valid = false;
    feature_config.feature_used_event = feature_used_event;
    return feature_config;
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
  EditableConfiguration configuration_;
};

bool IsSame(const FeatureConfig& a, const FeatureConfig& b) {
  return a.valid == b.valid && a.feature_used_event == b.feature_used_event;
}

}  // namespace

TEST_F(EditableConfigurationTest, SingleConfigAddAndGet) {
  scoped_feature_list_.InitWithFeatures({kTestFeatureFoo}, {});

  FeatureConfig foo_config = GetValidFeatureConfig("foo");
  configuration_.SetConfiguration(&kTestFeatureFoo, foo_config);
  const FeatureConfig& foo_config_result =
      configuration_.GetFeatureConfig(&kTestFeatureFoo);

  EXPECT_TRUE(IsSame(foo_config, foo_config_result));
}

TEST_F(EditableConfigurationTest, TwoConfigAddAndGet) {
  scoped_feature_list_.InitWithFeatures({kTestFeatureFoo, kTestFeatureBar}, {});

  FeatureConfig foo_config = GetValidFeatureConfig("foo");
  configuration_.SetConfiguration(&kTestFeatureFoo, foo_config);
  FeatureConfig bar_config = GetValidFeatureConfig("bar");
  configuration_.SetConfiguration(&kTestFeatureBar, bar_config);

  const FeatureConfig& foo_config_result =
      configuration_.GetFeatureConfig(&kTestFeatureFoo);
  const FeatureConfig& bar_config_result =
      configuration_.GetFeatureConfig(&kTestFeatureBar);

  EXPECT_TRUE(IsSame(foo_config, foo_config_result));
  EXPECT_TRUE(IsSame(bar_config, bar_config_result));
}

TEST_F(EditableConfigurationTest, ConfigShouldBeEditable) {
  scoped_feature_list_.InitWithFeatures({kTestFeatureFoo}, {});

  FeatureConfig valid_foo_config = GetValidFeatureConfig("foo");
  configuration_.SetConfiguration(&kTestFeatureFoo, valid_foo_config);

  const FeatureConfig& valid_foo_config_result =
      configuration_.GetFeatureConfig(&kTestFeatureFoo);
  EXPECT_TRUE(IsSame(valid_foo_config, valid_foo_config_result));

  FeatureConfig invalid_foo_config = GetInvalidFeatureConfig("foo2");
  configuration_.SetConfiguration(&kTestFeatureFoo, invalid_foo_config);
  const FeatureConfig& invalid_foo_config_result =
      configuration_.GetFeatureConfig(&kTestFeatureFoo);
  EXPECT_TRUE(IsSame(invalid_foo_config, invalid_foo_config_result));
}

}  // namespace feature_engagement_tracker
