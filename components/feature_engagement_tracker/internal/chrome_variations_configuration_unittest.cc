// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement_tracker/internal/chrome_variations_configuration.h"

#include <map>
#include <string>
#include <vector>

#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_param_associator.h"
#include "base/metrics/field_trial_params.h"
#include "base/test/scoped_feature_list.h"
#include "components/feature_engagement_tracker/internal/configuration.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace feature_engagement_tracker {

namespace {

const base::Feature kTestFeatureFoo{"test_foo",
                                    base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kTestFeatureBar{"test_bar",
                                    base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kTestFeatureQux{"test_qux",
                                    base::FEATURE_DISABLED_BY_DEFAULT};

const char kFooTrialName[] = "FooTrial";
const char kBarTrialName[] = "BarTrial";
const char kQuxTrialName[] = "QuxTrial";
const char kGroupName[] = "Group1";

Comparator CreateComparator(ComparatorType type, uint32_t value) {
  Comparator comparator;
  comparator.type = type;
  comparator.value = value;
  return comparator;
}

PreconditionConfig CreatePreconditionConfig(const std::string& event_name,
                                            ComparatorType type,
                                            uint32_t value,
                                            uint32_t window,
                                            uint32_t storage) {
  PreconditionConfig precondition_config;
  precondition_config.event_name = event_name;
  precondition_config.comparator = CreateComparator(type, value);
  precondition_config.window = window;
  precondition_config.storage = storage;
  return precondition_config;
}

class ChromeVariationsConfigurationTest : public ::testing::Test {
 public:
  ChromeVariationsConfigurationTest() : field_trials_(nullptr) {
    base::FieldTrial* foo_trial =
        base::FieldTrialList::CreateFieldTrial(kFooTrialName, kGroupName);
    base::FieldTrial* bar_trial =
        base::FieldTrialList::CreateFieldTrial(kBarTrialName, kGroupName);
    base::FieldTrial* qux_trial =
        base::FieldTrialList::CreateFieldTrial(kQuxTrialName, kGroupName);
    trials_[&kTestFeatureFoo] = foo_trial;
    trials_[&kTestFeatureBar] = bar_trial;
    trials_[&kTestFeatureQux] = qux_trial;

    std::unique_ptr<base::FeatureList> feature_list(new base::FeatureList);
    feature_list->RegisterFieldTrialOverride(
        kTestFeatureFoo.name, base::FeatureList::OVERRIDE_ENABLE_FEATURE,
        foo_trial);
    feature_list->RegisterFieldTrialOverride(
        kTestFeatureBar.name, base::FeatureList::OVERRIDE_ENABLE_FEATURE,
        bar_trial);
    feature_list->RegisterFieldTrialOverride(
        kTestFeatureQux.name, base::FeatureList::OVERRIDE_ENABLE_FEATURE,
        qux_trial);

    scoped_feature_list.InitWithFeatureList(std::move(feature_list));
    EXPECT_EQ(foo_trial, base::FeatureList::GetFieldTrial(kTestFeatureFoo));
    EXPECT_EQ(bar_trial, base::FeatureList::GetFieldTrial(kTestFeatureBar));
    EXPECT_EQ(qux_trial, base::FeatureList::GetFieldTrial(kTestFeatureQux));
  }

  void TearDown() override {
    // This is required to ensure each test can define its own params.
    base::FieldTrialParamAssociator::GetInstance()->ClearAllParamsForTesting();
  }

 protected:
  void SetFeatureParams(const base::Feature& feature,
                        std::map<std::string, std::string> params) {
    ASSERT_TRUE(base::FieldTrialParamAssociator::GetInstance()
                    ->AssociateFieldTrialParams(trials_[&feature]->trial_name(),
                                                kGroupName, params));

    std::map<std::string, std::string> actualParams;
    EXPECT_TRUE(base::GetFieldTrialParamsByFeature(feature, &actualParams));
    EXPECT_EQ(params, actualParams);
  }

  void VerifyInvalid(const std::string& precondition);

  ChromeVariationsConfiguration configuration_;

 private:
  base::FieldTrialList field_trials_;
  std::map<const base::Feature*, base::FieldTrial*> trials_;
  base::test::ScopedFeatureList scoped_feature_list;

  DISALLOW_COPY_AND_ASSIGN(ChromeVariationsConfigurationTest);
};

}  // namespace

TEST_F(ChromeVariationsConfigurationTest,
       DisabledFeatureShouldHaveInvalidConfig) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeatures({}, {kTestFeatureFoo});

  FeatureVector features;
  features.push_back(&kTestFeatureFoo);

  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo_config = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_FALSE(foo_config.valid);
}

TEST_F(ChromeVariationsConfigurationTest, ParseSingleFeature) {
  std::map<std::string, std::string> foo_params;
  foo_params["precondition_used"] =
      "event:page_download_started;comparator:any;window:0;storage:360";
  foo_params["precondition_trigger"] =
      "event:opened_chrome_home;comparator:any;window:0;storage:360";
  foo_params["precondition_1"] =
      "event:user_has_seen_dino;comparator:>=1;window:120;storage:180";
  foo_params["precondition_2"] =
      "event:user_opened_app_menu;comparator:<=0;window:120;storage:180";
  foo_params["precondition_3"] =
      "event:user_opened_downloads_home;comparator:any;window:0;storage:360";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_TRUE(foo.valid);

  FeatureConfig expected_foo;
  expected_foo.valid = true;
  expected_foo.used =
      CreatePreconditionConfig("page_download_started", ANY, 0, 0, 360);
  expected_foo.trigger =
      CreatePreconditionConfig("opened_chrome_home", ANY, 0, 0, 360);
  expected_foo.preconditions.insert(CreatePreconditionConfig(
      "user_has_seen_dino", GREATER_THAN_OR_EQUAL, 1, 120, 180));
  expected_foo.preconditions.insert(CreatePreconditionConfig(
      "user_opened_app_menu", LESS_THAN_OR_EQUAL, 0, 120, 180));
  expected_foo.preconditions.insert(
      CreatePreconditionConfig("user_opened_downloads_home", ANY, 0, 0, 360));
  EXPECT_EQ(expected_foo, foo);
}

TEST_F(ChromeVariationsConfigurationTest, MissingUsedIsInvalid) {
  std::map<std::string, std::string> foo_params;
  foo_params["precondition_trigger"] =
      "event:et;comparator:any;window:0;storage:360";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_FALSE(foo.valid);
}

TEST_F(ChromeVariationsConfigurationTest, MissingTriggerIsInvalid) {
  std::map<std::string, std::string> foo_params;
  foo_params["precondition_used"] =
      "event:eu;comparator:any;window:0;storage:360";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_FALSE(foo.valid);
}

TEST_F(ChromeVariationsConfigurationTest, OnlyTriggerAndUsedIsValid) {
  std::map<std::string, std::string> foo_params;
  foo_params["precondition_used"] =
      "event:eu;comparator:any;window:0;storage:360";
  foo_params["precondition_trigger"] =
      "event:et;comparator:any;window:0;storage:360";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_TRUE(foo.valid);

  FeatureConfig expected_foo;
  expected_foo.valid = true;
  expected_foo.used = CreatePreconditionConfig("eu", ANY, 0, 0, 360);
  expected_foo.trigger = CreatePreconditionConfig("et", ANY, 0, 0, 360);
  EXPECT_EQ(expected_foo, foo);
}

TEST_F(ChromeVariationsConfigurationTest, WhitespaceIsValid) {
  std::map<std::string, std::string> foo_params;
  foo_params["precondition_used"] =
      " event : eu ; comparator : any ; window : 1 ; storage : 360 ";
  foo_params["precondition_trigger"] =
      " event:et;comparator : any ;window: 2;storage:360 ";
  foo_params["precondition_0"] =
      "event:e0;comparator: <1 ;window:3;storage:360";
  foo_params["precondition_1"] =
      "event:e1;comparator:  > 2 ;window:4;storage:360";
  foo_params["precondition_2"] =
      "event:e2;comparator: <= 3 ;window:5;storage:360";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_TRUE(foo.valid);

  FeatureConfig expected_foo;
  expected_foo.valid = true;
  expected_foo.used = CreatePreconditionConfig("eu", ANY, 0, 1, 360);
  expected_foo.trigger = CreatePreconditionConfig("et", ANY, 0, 2, 360);
  expected_foo.preconditions.insert(
      CreatePreconditionConfig("e0", LESS_THAN, 1, 3, 360));
  expected_foo.preconditions.insert(
      CreatePreconditionConfig("e1", GREATER_THAN, 2, 4, 360));
  expected_foo.preconditions.insert(
      CreatePreconditionConfig("e2", LESS_THAN_OR_EQUAL, 3, 5, 360));
  EXPECT_EQ(expected_foo, foo);
}

TEST_F(ChromeVariationsConfigurationTest, IgnoresInvalidConfigKeys) {
  std::map<std::string, std::string> foo_params;
  foo_params["precondition_used"] =
      "event:eu;comparator:any;window:0;storage:360";
  foo_params["precondition_trigger"] =
      "event:et;comparator:any;window:0;storage:360";
  foo_params["not_there_yet"] = "bogus value";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_TRUE(foo.valid);

  FeatureConfig expected_foo;
  expected_foo.valid = true;
  expected_foo.used = CreatePreconditionConfig("eu", ANY, 0, 0, 360);
  expected_foo.trigger = CreatePreconditionConfig("et", ANY, 0, 0, 360);
  EXPECT_EQ(expected_foo, foo);
}

TEST_F(ChromeVariationsConfigurationTest, IgnoresInvalidPreconditionTokens) {
  std::map<std::string, std::string> foo_params;
  foo_params["precondition_used"] =
      "event:eu;comparator:any;window:0;storage:360;somethingelse:1";
  foo_params["precondition_trigger"] =
      "yesway:0;noway:1;event:et;comparator:any;window:0;storage:360";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_TRUE(foo.valid);

  FeatureConfig expected_foo;
  expected_foo.valid = true;
  expected_foo.used = CreatePreconditionConfig("eu", ANY, 0, 0, 360);
  expected_foo.trigger = CreatePreconditionConfig("et", ANY, 0, 0, 360);
  EXPECT_EQ(expected_foo, foo);
}

void ChromeVariationsConfigurationTest::VerifyInvalid(
    const std::string& precondition) {
  std::map<std::string, std::string> foo_params;
  foo_params["precondition_used"] = "event:u;comparator:any;window:0;storage:1";
  foo_params["precondition_trigger"] =
      "event:et;comparator:any;window:0;storage:360";
  foo_params["precondition_0"] = precondition;
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_FALSE(foo.valid);
}

TEST_F(ChromeVariationsConfigurationTest,
       MissingAllPreconditionParamsIsInvalid) {
  VerifyInvalid("a:1;b:2;c:3;d:4");
}

TEST_F(ChromeVariationsConfigurationTest,
       MissingEventPreconditionParamIsInvalid) {
  VerifyInvalid("foobar:eu;comparator:any;window:1;storage:360");
}

TEST_F(ChromeVariationsConfigurationTest,
       MissingComparatorPreconditionParamIsInvalid) {
  VerifyInvalid("event:eu;foobar:any;window:1;storage:360");
}

TEST_F(ChromeVariationsConfigurationTest,
       MissingWindowPreconditionParamIsInvalid) {
  VerifyInvalid("event:eu;comparator:any;foobar:1;storage:360");
}

TEST_F(ChromeVariationsConfigurationTest,
       MissingStoragePreconditionParamIsInvalid) {
  VerifyInvalid("event:eu;comparator:any;window:1;foobar:360");
}

TEST_F(ChromeVariationsConfigurationTest,
       EventSpecifiedMultipleTimesIsInvalid) {
  VerifyInvalid("event:eu;event:eu;comparator:any;window:1;storage:360");
}

TEST_F(ChromeVariationsConfigurationTest,
       ComparatorSpecifiedMultipleTimesIsInvalid) {
  VerifyInvalid("event:eu;comparator:any;comparator:any;window:1;storage:360");
}

TEST_F(ChromeVariationsConfigurationTest,
       WindowSpecifiedMultipleTimesIsInvalid) {
  VerifyInvalid("event:eu;comparator:any;window:1;window:2;storage:360");
}

TEST_F(ChromeVariationsConfigurationTest,
       StorageSpecifiedMultipleTimesIsInvalid) {
  VerifyInvalid("event:eu;comparator:any;window:1;storage:360;storage:10");
}

TEST_F(ChromeVariationsConfigurationTest,
       InvalidSessionRateCausesInvalidConfig) {
  std::map<std::string, std::string> foo_params;
  foo_params["precondition_used"] =
      "event:eu;comparator:any;window:1;storage:360";
  foo_params["precondition_trigger"] =
      "event:et;comparator:any;window:0;storage:360";
  foo_params["session_rate"] = "bogus value";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_FALSE(foo.valid);
}

TEST_F(ChromeVariationsConfigurationTest,
       InvalidAvailabilityCausesInvalidConfig) {
  std::map<std::string, std::string> foo_params;
  foo_params["precondition_used"] =
      "event:eu;comparator:any;window:0;storage:360";
  foo_params["precondition_trigger"] =
      "event:et;comparator:any;window:0;storage:360";
  foo_params["availability"] = "bogus value";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_FALSE(foo.valid);
}

TEST_F(ChromeVariationsConfigurationTest, InvalidUsedCausesInvalidConfig) {
  std::map<std::string, std::string> foo_params;
  foo_params["precondition_used"] = "bogus value";
  foo_params["precondition_trigger"] =
      "event:et;comparator:any;window:0;storage:360";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_FALSE(foo.valid);
}

TEST_F(ChromeVariationsConfigurationTest, InvalidTriggerCausesInvalidConfig) {
  std::map<std::string, std::string> foo_params;
  foo_params["precondition_used"] =
      "event:eu;comparator:any;window:0;storage:360";
  foo_params["precondition_trigger"] = "bogus value";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_FALSE(foo.valid);
}

TEST_F(ChromeVariationsConfigurationTest,
       InvalidPreconditionCausesInvalidConfig) {
  std::map<std::string, std::string> foo_params;
  foo_params["precondition_used"] =
      "event:eu;comparator:any;window:0;storage:360";
  foo_params["precondition_trigger"] =
      "event:et;comparator:any;window:0;storage:360";
  foo_params["precondition_used_0"] = "bogus value";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_FALSE(foo.valid);
}

TEST_F(ChromeVariationsConfigurationTest, AllComparatorTypesWork) {
  std::map<std::string, std::string> foo_params;
  foo_params["precondition_used"] =
      "event:e0;comparator:any;window:20;storage:30";
  foo_params["precondition_trigger"] =
      "event:e1;comparator:<1;window:21;storage:31";
  foo_params["precondition_2"] = "event:e2;comparator:>2;window:22;storage:32";
  foo_params["precondition_3"] = "event:e3;comparator:<=3;window:23;storage:33";
  foo_params["precondition_4"] = "event:e4;comparator:>=4;window:24;storage:34";
  foo_params["precondition_5"] = "event:e5;comparator:==5;window:25;storage:35";
  foo_params["precondition_6"] = "event:e6;comparator:!=6;window:26;storage:36";
  foo_params["session_rate"] = "!=6";
  foo_params["availability"] = ">=1";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_TRUE(foo.valid);

  FeatureConfig expected_foo;
  expected_foo.valid = true;
  expected_foo.used = CreatePreconditionConfig("e0", ANY, 0, 20, 30);
  expected_foo.trigger = CreatePreconditionConfig("e1", LESS_THAN, 1, 21, 31);
  expected_foo.preconditions.insert(
      CreatePreconditionConfig("e2", GREATER_THAN, 2, 22, 32));
  expected_foo.preconditions.insert(
      CreatePreconditionConfig("e3", LESS_THAN_OR_EQUAL, 3, 23, 33));
  expected_foo.preconditions.insert(
      CreatePreconditionConfig("e4", GREATER_THAN_OR_EQUAL, 4, 24, 34));
  expected_foo.preconditions.insert(
      CreatePreconditionConfig("e5", EQUAL, 5, 25, 35));
  expected_foo.preconditions.insert(
      CreatePreconditionConfig("e6", NOT_EQUAL, 6, 26, 36));
  expected_foo.session_rate = CreateComparator(NOT_EQUAL, 6);
  expected_foo.availability = CreateComparator(GREATER_THAN_OR_EQUAL, 1);
  EXPECT_EQ(expected_foo, foo);
}

TEST_F(ChromeVariationsConfigurationTest, ParseMultipleFeatures) {
  std::map<std::string, std::string> foo_params;
  foo_params["precondition_used"] =
      "event:foo_used;comparator:any;window:20;storage:30";
  foo_params["precondition_trigger"] =
      "event:foo_trigger;comparator:<1;window:21;storage:31";
  foo_params["session_rate"] = "==10";
  foo_params["availability"] = "<0";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::map<std::string, std::string> bar_params;
  bar_params["precondition_used"] =
      "event:bar_used;comparator:ANY;window:0;storage:0";
  SetFeatureParams(kTestFeatureBar, bar_params);

  std::map<std::string, std::string> qux_params;
  qux_params["precondition_used"] =
      "event:qux_used;comparator:>=2;window:99;storage:42";
  qux_params["precondition_trigger"] =
      "event:qux_trigger;comparator:ANY;window:103;storage:104";
  qux_params["precondition_0"] = "event:q0;comparator:<10;window:20;storage:30";
  qux_params["precondition_1"] = "event:q1;comparator:>11;window:21;storage:31";
  qux_params["precondition_2"] =
      "event:q2;comparator:<=12;window:22;storage:32";
  qux_params["precondition_3"] =
      "event:q3;comparator:>=13;window:23;storage:33";
  qux_params["precondition_4"] =
      "event:q4;comparator:==14;window:24;storage:34";
  qux_params["precondition_5"] =
      "event:q5;comparator:!=15;window:25;storage:35";
  qux_params["session_rate"] = "!=13";
  qux_params["availability"] = "==0";
  SetFeatureParams(kTestFeatureQux, qux_params);

  std::vector<const base::Feature*> features = {
      &kTestFeatureFoo, &kTestFeatureBar, &kTestFeatureQux};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_TRUE(foo.valid);
  FeatureConfig expected_foo;
  expected_foo.valid = true;
  expected_foo.used = CreatePreconditionConfig("foo_used", ANY, 0, 20, 30);
  expected_foo.trigger =
      CreatePreconditionConfig("foo_trigger", LESS_THAN, 1, 21, 31);
  expected_foo.session_rate = CreateComparator(EQUAL, 10);
  expected_foo.availability = CreateComparator(LESS_THAN, 0);
  EXPECT_EQ(expected_foo, foo);

  FeatureConfig bar = configuration_.GetFeatureConfig(kTestFeatureBar);
  EXPECT_FALSE(bar.valid);

  FeatureConfig qux = configuration_.GetFeatureConfig(kTestFeatureQux);
  EXPECT_TRUE(qux.valid);
  FeatureConfig expected_qux;
  expected_qux.valid = true;
  expected_qux.used =
      CreatePreconditionConfig("qux_used", GREATER_THAN_OR_EQUAL, 2, 99, 42);
  expected_qux.trigger =
      CreatePreconditionConfig("qux_trigger", ANY, 0, 103, 104);
  expected_qux.preconditions.insert(
      CreatePreconditionConfig("q0", LESS_THAN, 10, 20, 30));
  expected_qux.preconditions.insert(
      CreatePreconditionConfig("q1", GREATER_THAN, 11, 21, 31));
  expected_qux.preconditions.insert(
      CreatePreconditionConfig("q2", LESS_THAN_OR_EQUAL, 12, 22, 32));
  expected_qux.preconditions.insert(
      CreatePreconditionConfig("q3", GREATER_THAN_OR_EQUAL, 13, 23, 33));
  expected_qux.preconditions.insert(
      CreatePreconditionConfig("q4", EQUAL, 14, 24, 34));
  expected_qux.preconditions.insert(
      CreatePreconditionConfig("q5", NOT_EQUAL, 15, 25, 35));
  expected_qux.session_rate = CreateComparator(NOT_EQUAL, 13);
  expected_qux.availability = CreateComparator(EQUAL, 0);
  EXPECT_EQ(expected_qux, qux);
}

}  // namespace feature_engagement_tracker
