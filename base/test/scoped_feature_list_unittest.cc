// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/scoped_feature_list.h"

#include <map>
#include <string>
#include <utility>

#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace test {

namespace {

const Feature kTestFeature1{"TestFeature1", FEATURE_DISABLED_BY_DEFAULT};
const Feature kTestFeature2{"TestFeature2", FEATURE_DISABLED_BY_DEFAULT};

void ExpectFeatures(const std::string& enabled_features,
                    const std::string& disabled_features) {
  FeatureList* list = FeatureList::GetInstance();
  std::string actual_enabled_features;
  std::string actual_disabled_features;

  list->GetFeatureOverrides(&actual_enabled_features,
                            &actual_disabled_features);

  EXPECT_EQ(enabled_features, actual_enabled_features);
  EXPECT_EQ(disabled_features, actual_disabled_features);
}

}  // namespace

class ScopedFeatureListTest : public testing::Test {
 public:
  ScopedFeatureListTest() {
    // Clear default feature list.
    std::unique_ptr<FeatureList> feature_list(new FeatureList);
    feature_list->InitializeFromCommandLine(std::string(), std::string());
    original_feature_list_ = FeatureList::ClearInstanceForTesting();
    FeatureList::SetInstance(std::move(feature_list));
  }

  ~ScopedFeatureListTest() override {
    // Restore feature list.
    if (original_feature_list_) {
      FeatureList::ClearInstanceForTesting();
      FeatureList::RestoreInstanceForTesting(std::move(original_feature_list_));
    }
  }

 private:
  // Save the present FeatureList and restore it after test finish.
  std::unique_ptr<FeatureList> original_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(ScopedFeatureListTest);
};

TEST_F(ScopedFeatureListTest, BasicScoped) {
  ExpectFeatures(std::string(), std::string());
  EXPECT_FALSE(FeatureList::IsEnabled(kTestFeature1));
  {
    test::ScopedFeatureList feature_list1;
    feature_list1.InitFromCommandLine("TestFeature1", std::string());
    ExpectFeatures("TestFeature1", std::string());
    EXPECT_TRUE(FeatureList::IsEnabled(kTestFeature1));
  }
  ExpectFeatures(std::string(), std::string());
  EXPECT_FALSE(FeatureList::IsEnabled(kTestFeature1));
}

TEST_F(ScopedFeatureListTest, EnableWithTrial) {
  FieldTrialList trial_list(nullptr);
  FieldTrial* trial =
      FieldTrialList::CreateFieldTrial("test_name", "test_group");
  test::ScopedFeatureList feature_list1;
  feature_list1.InitAndEnableFeatureWithFieldTrialOverride(kTestFeature1,
                                                           trial);
  ExpectFeatures("TestFeature1<test_name", std::string());
  EXPECT_TRUE(FeatureList::IsEnabled(kTestFeature1));
  EXPECT_EQ(trial, FeatureList::GetFieldTrial(kTestFeature1));
}

TEST_F(ScopedFeatureListTest, OverrideWithTrial) {
  FieldTrialList trial_list(nullptr);
  FieldTrial* trial =
      FieldTrialList::CreateFieldTrial("test_name", "test_group");

  test::ScopedFeatureList feature_list1;
  feature_list1.InitFromCommandLine("TestFeature1", std::string());
  ExpectFeatures("TestFeature1", std::string());
  EXPECT_EQ(nullptr, FeatureList::GetFieldTrial(kTestFeature1));

  {
    test::ScopedFeatureList feature_list2;
    feature_list2.InitAndEnableFeatureWithFieldTrialOverride(kTestFeature1,
                                                             trial);
    ExpectFeatures("TestFeature1<test_name", std::string());
    EXPECT_TRUE(FeatureList::IsEnabled(kTestFeature1));
    EXPECT_EQ(trial, FeatureList::GetFieldTrial(kTestFeature1));
  }
}

TEST_F(ScopedFeatureListTest, OverrideWithFeatureParameters) {
  FieldTrialList trial_list(nullptr);
  const char kParam1[] = "param_1";
  const char kParam2[] = "param_2";
  const char kValue1[] = "value_1";
  const char kValue2[] = "value_2";
  std::map<std::string, std::string> parameters;
  parameters[kParam1] = kValue1;
  parameters[kParam2] = kValue2;

  ExpectFeatures(std::string(), std::string());
  EXPECT_EQ(nullptr, FeatureList::GetFieldTrial(kTestFeature1));
  EXPECT_EQ("", GetFieldTrialParamValueByFeature(kTestFeature1, kParam1));
  EXPECT_EQ("", GetFieldTrialParamValueByFeature(kTestFeature1, kParam2));

  {
    test::ScopedFeatureList feature_list;

    feature_list.InitAndEnableFeatureWithParameters(kTestFeature1, parameters);
    EXPECT_TRUE(FeatureList::IsEnabled(kTestFeature1));
    EXPECT_EQ(kValue1,
              GetFieldTrialParamValueByFeature(kTestFeature1, kParam1));
    EXPECT_EQ(kValue2,
              GetFieldTrialParamValueByFeature(kTestFeature1, kParam2));
  }

  ExpectFeatures(std::string(), std::string());
  EXPECT_EQ(nullptr, FeatureList::GetFieldTrial(kTestFeature1));
  EXPECT_EQ("", GetFieldTrialParamValueByFeature(kTestFeature1, kParam1));
  EXPECT_EQ("", GetFieldTrialParamValueByFeature(kTestFeature1, kParam2));
}

TEST_F(ScopedFeatureListTest, InitWithFeaturesAndTrials) {
  FieldTrialList trial_list(nullptr);
  FieldTrial* trial_1 = FieldTrialList::CreateFieldTrial("name1", "group1");
  FieldTrial* trial_2 = FieldTrialList::CreateFieldTrial("name2", "group2");

  test::ScopedFeatureList feature_list1;
  feature_list1.InitFromCommandLine("TestFeature1,TestFeature2<name2",
                                    "TestFeature3");
  ExpectFeatures("TestFeature1,TestFeature2<name2", "TestFeature3");
  EXPECT_EQ(nullptr, FeatureList::GetFieldTrial(kTestFeature1));
  EXPECT_EQ(trial_2, FeatureList::GetFieldTrial(kTestFeature2));

  {
    test::ScopedFeatureList feature_list2;
    feature_list2.InitWithFeaturesAndFieldTrials({kTestFeature1, kTestFeature2},
                                                 {trial_2, trial_1}, {});
    ExpectFeatures("TestFeature1<name2,TestFeature2<name1", "TestFeature3");
    EXPECT_TRUE(FeatureList::IsEnabled(kTestFeature1));
    EXPECT_EQ(trial_2, FeatureList::GetFieldTrial(kTestFeature1));
    EXPECT_TRUE(FeatureList::IsEnabled(kTestFeature2));
    EXPECT_EQ(trial_1, FeatureList::GetFieldTrial(kTestFeature2));
  }

  ExpectFeatures("TestFeature1,TestFeature2<name2", "TestFeature3");
  EXPECT_EQ(nullptr, FeatureList::GetFieldTrial(kTestFeature1));
  EXPECT_EQ(trial_2, FeatureList::GetFieldTrial(kTestFeature2));

  {
    test::ScopedFeatureList feature_list2;
    feature_list2.InitWithFeaturesAndFieldTrials({kTestFeature1}, {trial_1},
                                                 {});
    ExpectFeatures("TestFeature1<name1,TestFeature2<name2", "TestFeature3");
    EXPECT_TRUE(FeatureList::IsEnabled(kTestFeature1));
    EXPECT_EQ(trial_1, FeatureList::GetFieldTrial(kTestFeature1));
    EXPECT_TRUE(FeatureList::IsEnabled(kTestFeature2));
    EXPECT_EQ(trial_2, FeatureList::GetFieldTrial(kTestFeature2));
  }

  ExpectFeatures("TestFeature1,TestFeature2<name2", "TestFeature3");
  EXPECT_EQ(nullptr, FeatureList::GetFieldTrial(kTestFeature1));
  EXPECT_EQ(trial_2, FeatureList::GetFieldTrial(kTestFeature2));
}

TEST_F(ScopedFeatureListTest, EnableFeatureOverrideDisable) {
  test::ScopedFeatureList feature_list1;
  feature_list1.InitWithFeatures({}, {kTestFeature1});

  {
    test::ScopedFeatureList feature_list2;
    feature_list2.InitWithFeatures({kTestFeature1}, {});
    ExpectFeatures("TestFeature1", std::string());
  }
}

TEST_F(ScopedFeatureListTest, FeatureOverrideNotMakeDuplicate) {
  test::ScopedFeatureList feature_list1;
  feature_list1.InitWithFeatures({}, {kTestFeature1});

  {
    test::ScopedFeatureList feature_list2;
    feature_list2.InitWithFeatures({}, {kTestFeature1});
    ExpectFeatures(std::string(), "TestFeature1");
  }
}

TEST_F(ScopedFeatureListTest, FeatureOverrideFeatureWithDefault) {
  test::ScopedFeatureList feature_list1;
  feature_list1.InitFromCommandLine("*TestFeature1", std::string());

  {
    test::ScopedFeatureList feature_list2;
    feature_list2.InitWithFeatures({kTestFeature1}, {});
    ExpectFeatures("TestFeature1", std::string());
  }
}

TEST_F(ScopedFeatureListTest, FeatureOverrideFeatureWithDefault2) {
  test::ScopedFeatureList feature_list1;
  feature_list1.InitFromCommandLine("*TestFeature1", std::string());

  {
    test::ScopedFeatureList feature_list2;
    feature_list2.InitWithFeatures({}, {kTestFeature1});
    ExpectFeatures(std::string(), "TestFeature1");
  }
}

TEST_F(ScopedFeatureListTest, FeatureOverrideFeatureWithEnabledFieldTried) {
  test::ScopedFeatureList feature_list1;

  std::unique_ptr<FeatureList> feature_list(new FeatureList);
  FieldTrialList field_trial_list(nullptr);
  FieldTrial* trial = FieldTrialList::CreateFieldTrial("TrialExample", "A");
  feature_list->RegisterFieldTrialOverride(
      kTestFeature1.name, FeatureList::OVERRIDE_ENABLE_FEATURE, trial);
  feature_list1.InitWithFeatureList(std::move(feature_list));

  {
    test::ScopedFeatureList feature_list2;
    feature_list2.InitWithFeatures({kTestFeature1}, {});
    ExpectFeatures("TestFeature1", std::string());
  }
}

TEST_F(ScopedFeatureListTest, FeatureOverrideFeatureWithDisabledFieldTried) {
  test::ScopedFeatureList feature_list1;

  std::unique_ptr<FeatureList> feature_list(new FeatureList);
  FieldTrialList field_trial_list(nullptr);
  FieldTrial* trial = FieldTrialList::CreateFieldTrial("TrialExample", "A");
  feature_list->RegisterFieldTrialOverride(
      kTestFeature1.name, FeatureList::OVERRIDE_DISABLE_FEATURE, trial);
  feature_list1.InitWithFeatureList(std::move(feature_list));

  {
    test::ScopedFeatureList feature_list2;
    feature_list2.InitWithFeatures({kTestFeature1}, {});
    ExpectFeatures("TestFeature1", std::string());
  }
}

TEST_F(ScopedFeatureListTest, FeatureOverrideKeepsOtherExistingFeature) {
  test::ScopedFeatureList feature_list1;
  feature_list1.InitWithFeatures({}, {kTestFeature1});

  {
    test::ScopedFeatureList feature_list2;
    feature_list2.InitWithFeatures({}, {kTestFeature2});
    EXPECT_FALSE(FeatureList::IsEnabled(kTestFeature1));
    EXPECT_FALSE(FeatureList::IsEnabled(kTestFeature2));
  }
}

TEST_F(ScopedFeatureListTest, FeatureOverrideKeepsOtherExistingFeature2) {
  test::ScopedFeatureList feature_list1;
  feature_list1.InitWithFeatures({}, {kTestFeature1});

  {
    test::ScopedFeatureList feature_list2;
    feature_list2.InitWithFeatures({kTestFeature2}, {});
    ExpectFeatures("TestFeature2", "TestFeature1");
  }
}

TEST_F(ScopedFeatureListTest, FeatureOverrideKeepsOtherExistingDefaultFeature) {
  test::ScopedFeatureList feature_list1;
  feature_list1.InitFromCommandLine("*TestFeature1", std::string());

  {
    test::ScopedFeatureList feature_list2;
    feature_list2.InitWithFeatures({}, {kTestFeature2});
    ExpectFeatures("*TestFeature1", "TestFeature2");
  }
}

}  // namespace test
}  // namespace base
