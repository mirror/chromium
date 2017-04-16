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

EventConfig CreateEventConfig(const std::string& name,
                              ComparatorType type,
                              uint32_t value,
                              uint32_t window,
                              uint32_t storage) {
  EventConfig event_config;
  event_config.name = name;
  event_config.comparator = CreateComparator(type, value);
  event_config.window = window;
  event_config.storage = storage;
  return event_config;
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

  void VerifyInvalid(const std::string& event_config);

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
  foo_params["event_used"] =
      "name:page_download_started;comparator:any;window:0;storage:360";
  foo_params["event_trigger"] =
      "name:opened_chrome_home;comparator:any;window:0;storage:360";
  foo_params["event_1"] =
      "name:user_has_seen_dino;comparator:>=1;window:120;storage:180";
  foo_params["event_2"] =
      "name:user_opened_app_menu;comparator:<=0;window:120;storage:180";
  foo_params["event_3"] =
      "name:user_opened_downloads_home;comparator:any;window:0;storage:360";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_TRUE(foo.valid);

  FeatureConfig expected_foo;
  expected_foo.valid = true;
  expected_foo.used =
      CreateEventConfig("page_download_started", ANY, 0, 0, 360);
  expected_foo.trigger =
      CreateEventConfig("opened_chrome_home", ANY, 0, 0, 360);
  expected_foo.event_configs.insert(CreateEventConfig(
      "user_has_seen_dino", GREATER_THAN_OR_EQUAL, 1, 120, 180));
  expected_foo.event_configs.insert(CreateEventConfig(
      "user_opened_app_menu", LESS_THAN_OR_EQUAL, 0, 120, 180));
  expected_foo.event_configs.insert(
      CreateEventConfig("user_opened_downloads_home", ANY, 0, 0, 360));
  EXPECT_EQ(expected_foo, foo);
}

TEST_F(ChromeVariationsConfigurationTest, MissingUsedIsInvalid) {
  std::map<std::string, std::string> foo_params;
  foo_params["event_trigger"] = "name:et;comparator:any;window:0;storage:360";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_FALSE(foo.valid);
}

TEST_F(ChromeVariationsConfigurationTest, MissingTriggerIsInvalid) {
  std::map<std::string, std::string> foo_params;
  foo_params["event_used"] = "name:eu;comparator:any;window:0;storage:360";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_FALSE(foo.valid);
}

TEST_F(ChromeVariationsConfigurationTest, OnlyTriggerAndUsedIsValid) {
  std::map<std::string, std::string> foo_params;
  foo_params["event_used"] = "name:eu;comparator:any;window:0;storage:360";
  foo_params["event_trigger"] = "name:et;comparator:any;window:0;storage:360";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_TRUE(foo.valid);

  FeatureConfig expected_foo;
  expected_foo.valid = true;
  expected_foo.used = CreateEventConfig("eu", ANY, 0, 0, 360);
  expected_foo.trigger = CreateEventConfig("et", ANY, 0, 0, 360);
  EXPECT_EQ(expected_foo, foo);
}

TEST_F(ChromeVariationsConfigurationTest, WhitespaceIsValid) {
  std::map<std::string, std::string> foo_params;
  foo_params["event_used"] =
      " name : eu ; comparator : any ; window : 1 ; storage : 360 ";
  foo_params["event_trigger"] =
      " name:et;comparator : any ;window: 2;storage:360 ";
  foo_params["event_0"] = "name:e0;comparator: <1 ;window:3;storage:360";
  foo_params["event_1"] = "name:e1;comparator:  > 2 ;window:4;storage:360";
  foo_params["event_2"] = "name:e2;comparator: <= 3 ;window:5;storage:360";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_TRUE(foo.valid);

  FeatureConfig expected_foo;
  expected_foo.valid = true;
  expected_foo.used = CreateEventConfig("eu", ANY, 0, 1, 360);
  expected_foo.trigger = CreateEventConfig("et", ANY, 0, 2, 360);
  expected_foo.event_configs.insert(
      CreateEventConfig("e0", LESS_THAN, 1, 3, 360));
  expected_foo.event_configs.insert(
      CreateEventConfig("e1", GREATER_THAN, 2, 4, 360));
  expected_foo.event_configs.insert(
      CreateEventConfig("e2", LESS_THAN_OR_EQUAL, 3, 5, 360));
  EXPECT_EQ(expected_foo, foo);
}

TEST_F(ChromeVariationsConfigurationTest, IgnoresInvalidConfigKeys) {
  std::map<std::string, std::string> foo_params;
  foo_params["event_used"] = "name:eu;comparator:any;window:0;storage:360";
  foo_params["event_trigger"] = "name:et;comparator:any;window:0;storage:360";
  foo_params["not_there_yet"] = "bogus value";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_TRUE(foo.valid);

  FeatureConfig expected_foo;
  expected_foo.valid = true;
  expected_foo.used = CreateEventConfig("eu", ANY, 0, 0, 360);
  expected_foo.trigger = CreateEventConfig("et", ANY, 0, 0, 360);
  EXPECT_EQ(expected_foo, foo);
}

TEST_F(ChromeVariationsConfigurationTest, IgnoresInvalidEventConfigTokens) {
  std::map<std::string, std::string> foo_params;
  foo_params["event_used"] =
      "name:eu;comparator:any;window:0;storage:360;somethingelse:1";
  foo_params["event_trigger"] =
      "yesway:0;noway:1;name:et;comparator:any;window:0;storage:360";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_TRUE(foo.valid);

  FeatureConfig expected_foo;
  expected_foo.valid = true;
  expected_foo.used = CreateEventConfig("eu", ANY, 0, 0, 360);
  expected_foo.trigger = CreateEventConfig("et", ANY, 0, 0, 360);
  EXPECT_EQ(expected_foo, foo);
}

void ChromeVariationsConfigurationTest::VerifyInvalid(
    const std::string& event_config) {
  std::map<std::string, std::string> foo_params;
  foo_params["event_used"] = "name:u;comparator:any;window:0;storage:1";
  foo_params["event_trigger"] = "name:et;comparator:any;window:0;storage:360";
  foo_params["event_0"] = event_config;
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_FALSE(foo.valid);
}

TEST_F(ChromeVariationsConfigurationTest,
       MissingAllEventConfigParamsIsInvalid) {
  VerifyInvalid("a:1;b:2;c:3;d:4");
}

TEST_F(ChromeVariationsConfigurationTest,
       MissingEventEventConfigParamIsInvalid) {
  VerifyInvalid("foobar:eu;comparator:any;window:1;storage:360");
}

TEST_F(ChromeVariationsConfigurationTest,
       MissingComparatorEventConfigParamIsInvalid) {
  VerifyInvalid("name:eu;foobar:any;window:1;storage:360");
}

TEST_F(ChromeVariationsConfigurationTest,
       MissingWindowEventConfigParamIsInvalid) {
  VerifyInvalid("name:eu;comparator:any;foobar:1;storage:360");
}

TEST_F(ChromeVariationsConfigurationTest,
       MissingStorageEventConfigParamIsInvalid) {
  VerifyInvalid("name:eu;comparator:any;window:1;foobar:360");
}

TEST_F(ChromeVariationsConfigurationTest,
       EventSpecifiedMultipleTimesIsInvalid) {
  VerifyInvalid("name:eu;name:eu;comparator:any;window:1;storage:360");
}

TEST_F(ChromeVariationsConfigurationTest,
       ComparatorSpecifiedMultipleTimesIsInvalid) {
  VerifyInvalid("name:eu;comparator:any;comparator:any;window:1;storage:360");
}

TEST_F(ChromeVariationsConfigurationTest,
       WindowSpecifiedMultipleTimesIsInvalid) {
  VerifyInvalid("name:eu;comparator:any;window:1;window:2;storage:360");
}

TEST_F(ChromeVariationsConfigurationTest,
       StorageSpecifiedMultipleTimesIsInvalid) {
  VerifyInvalid("name:eu;comparator:any;window:1;storage:360;storage:10");
}

TEST_F(ChromeVariationsConfigurationTest,
       InvalidSessionRateCausesInvalidConfig) {
  std::map<std::string, std::string> foo_params;
  foo_params["event_used"] = "name:eu;comparator:any;window:1;storage:360";
  foo_params["event_trigger"] = "name:et;comparator:any;window:0;storage:360";
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
  foo_params["event_used"] = "name:eu;comparator:any;window:0;storage:360";
  foo_params["event_trigger"] = "name:et;comparator:any;window:0;storage:360";
  foo_params["availability"] = "bogus value";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_FALSE(foo.valid);
}

TEST_F(ChromeVariationsConfigurationTest, InvalidUsedCausesInvalidConfig) {
  std::map<std::string, std::string> foo_params;
  foo_params["event_used"] = "bogus value";
  foo_params["event_trigger"] = "name:et;comparator:any;window:0;storage:360";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_FALSE(foo.valid);
}

TEST_F(ChromeVariationsConfigurationTest, InvalidTriggerCausesInvalidConfig) {
  std::map<std::string, std::string> foo_params;
  foo_params["event_used"] = "name:eu;comparator:any;window:0;storage:360";
  foo_params["event_trigger"] = "bogus value";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_FALSE(foo.valid);
}

TEST_F(ChromeVariationsConfigurationTest,
       InvalidEventConfigCausesInvalidConfig) {
  std::map<std::string, std::string> foo_params;
  foo_params["event_used"] = "name:eu;comparator:any;window:0;storage:360";
  foo_params["event_trigger"] = "name:et;comparator:any;window:0;storage:360";
  foo_params["event_used_0"] = "bogus value";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_FALSE(foo.valid);
}

TEST_F(ChromeVariationsConfigurationTest, AllComparatorTypesWork) {
  std::map<std::string, std::string> foo_params;
  foo_params["event_used"] = "name:e0;comparator:any;window:20;storage:30";
  foo_params["event_trigger"] = "name:e1;comparator:<1;window:21;storage:31";
  foo_params["event_2"] = "name:e2;comparator:>2;window:22;storage:32";
  foo_params["event_3"] = "name:e3;comparator:<=3;window:23;storage:33";
  foo_params["event_4"] = "name:e4;comparator:>=4;window:24;storage:34";
  foo_params["event_5"] = "name:e5;comparator:==5;window:25;storage:35";
  foo_params["event_6"] = "name:e6;comparator:!=6;window:26;storage:36";
  foo_params["session_rate"] = "!=6";
  foo_params["availability"] = ">=1";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::vector<const base::Feature*> features = {&kTestFeatureFoo};
  configuration_.ParseFeatureConfigs(features);

  FeatureConfig foo = configuration_.GetFeatureConfig(kTestFeatureFoo);
  EXPECT_TRUE(foo.valid);

  FeatureConfig expected_foo;
  expected_foo.valid = true;
  expected_foo.used = CreateEventConfig("e0", ANY, 0, 20, 30);
  expected_foo.trigger = CreateEventConfig("e1", LESS_THAN, 1, 21, 31);
  expected_foo.event_configs.insert(
      CreateEventConfig("e2", GREATER_THAN, 2, 22, 32));
  expected_foo.event_configs.insert(
      CreateEventConfig("e3", LESS_THAN_OR_EQUAL, 3, 23, 33));
  expected_foo.event_configs.insert(
      CreateEventConfig("e4", GREATER_THAN_OR_EQUAL, 4, 24, 34));
  expected_foo.event_configs.insert(CreateEventConfig("e5", EQUAL, 5, 25, 35));
  expected_foo.event_configs.insert(
      CreateEventConfig("e6", NOT_EQUAL, 6, 26, 36));
  expected_foo.session_rate = CreateComparator(NOT_EQUAL, 6);
  expected_foo.availability = CreateComparator(GREATER_THAN_OR_EQUAL, 1);
  EXPECT_EQ(expected_foo, foo);
}

TEST_F(ChromeVariationsConfigurationTest, ParseMultipleFeatures) {
  std::map<std::string, std::string> foo_params;
  foo_params["event_used"] =
      "name:foo_used;comparator:any;window:20;storage:30";
  foo_params["event_trigger"] =
      "name:foo_trigger;comparator:<1;window:21;storage:31";
  foo_params["session_rate"] = "==10";
  foo_params["availability"] = "<0";
  SetFeatureParams(kTestFeatureFoo, foo_params);

  std::map<std::string, std::string> bar_params;
  bar_params["event_used"] = "name:bar_used;comparator:ANY;window:0;storage:0";
  SetFeatureParams(kTestFeatureBar, bar_params);

  std::map<std::string, std::string> qux_params;
  qux_params["event_used"] =
      "name:qux_used;comparator:>=2;window:99;storage:42";
  qux_params["event_trigger"] =
      "name:qux_trigger;comparator:ANY;window:103;storage:104";
  qux_params["event_0"] = "name:q0;comparator:<10;window:20;storage:30";
  qux_params["event_1"] = "name:q1;comparator:>11;window:21;storage:31";
  qux_params["event_2"] = "name:q2;comparator:<=12;window:22;storage:32";
  qux_params["event_3"] = "name:q3;comparator:>=13;window:23;storage:33";
  qux_params["event_4"] = "name:q4;comparator:==14;window:24;storage:34";
  qux_params["event_5"] = "name:q5;comparator:!=15;window:25;storage:35";
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
  expected_foo.used = CreateEventConfig("foo_used", ANY, 0, 20, 30);
  expected_foo.trigger = CreateEventConfig("foo_trigger", LESS_THAN, 1, 21, 31);
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
      CreateEventConfig("qux_used", GREATER_THAN_OR_EQUAL, 2, 99, 42);
  expected_qux.trigger = CreateEventConfig("qux_trigger", ANY, 0, 103, 104);
  expected_qux.event_configs.insert(
      CreateEventConfig("q0", LESS_THAN, 10, 20, 30));
  expected_qux.event_configs.insert(
      CreateEventConfig("q1", GREATER_THAN, 11, 21, 31));
  expected_qux.event_configs.insert(
      CreateEventConfig("q2", LESS_THAN_OR_EQUAL, 12, 22, 32));
  expected_qux.event_configs.insert(
      CreateEventConfig("q3", GREATER_THAN_OR_EQUAL, 13, 23, 33));
  expected_qux.event_configs.insert(CreateEventConfig("q4", EQUAL, 14, 24, 34));
  expected_qux.event_configs.insert(
      CreateEventConfig("q5", NOT_EQUAL, 15, 25, 35));
  expected_qux.session_rate = CreateComparator(NOT_EQUAL, 13);
  expected_qux.availability = CreateComparator(EQUAL, 0);
  EXPECT_EQ(expected_qux, qux);
}

}  // namespace feature_engagement_tracker
