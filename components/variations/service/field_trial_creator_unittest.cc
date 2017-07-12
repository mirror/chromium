// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/service/variations_field_trial_creator.h"

#include <stddef.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/feature_list.h"
#include "base/json/json_string_value_serializer.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/sha1.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/test/histogram_tester.h"
#include "base/version.h"
#include "components/prefs/testing_pref_service.h"
#include "components/variations/pref_names.h"
#include "components/variations/proto/study.pb.h"
#include "components/variations/proto/variations_seed.pb.h"
#include "components/variations/service/variations_service.h"
#include "components/web_resource/resource_request_allowed_notifier_test_util.h"
#include "net/base/url_util.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace variations {
namespace {

class TestVariationsServiceClient : public VariationsServiceClient {
 public:
  TestVariationsServiceClient() {}
  ~TestVariationsServiceClient() override {}

  // VariationsServiceClient:
  std::string GetApplicationLocale() override { return std::string(); }
  base::Callback<base::Version(void)> GetVersionForSimulationCallback()
      override {
    return base::Callback<base::Version(void)>();
  }
  net::URLRequestContextGetter* GetURLRequestContext() override {
    return nullptr;
  }
  network_time::NetworkTimeTracker* GetNetworkTimeTracker() override {
    return nullptr;
  }
  version_info::Channel GetChannel() override {
    return version_info::Channel::UNKNOWN;
  }
  bool OverridesRestrictParameter(std::string* parameter) override {
    if (restrict_parameter_.empty())
      return false;
    *parameter = restrict_parameter_;
    return true;
  }

  void set_restrict_parameter(const std::string& value) {
    restrict_parameter_ = value;
  }

 private:
  std::string restrict_parameter_;

  DISALLOW_COPY_AND_ASSIGN(TestVariationsServiceClient);
};

class TestVariationsFieldTrialCreator : public VariationsFieldTrialCreator {
 public:
  TestVariationsFieldTrialCreator(
      std::unique_ptr<web_resource::TestRequestAllowedNotifier> test_notifier,
      PrefService* local_state)
      : VariationsFieldTrialCreator(local_state,
                                    new TestVariationsServiceClient(),
                                    UIStringOverrider()) {
    SetCreateTrialsFromSeedCalledForTesting(true);
  }

  ~TestVariationsFieldTrialCreator() {}

  bool StoreSeed(const std::string& seed_data,
                 const std::string& seed_signature,
                 const std::string& country_code,
                 const base::Time& date_fetched,
                 bool is_delta_compressed,
                 bool is_gzip_compressed) {
    seed_stored_ = true;
    stored_seed_data_ = seed_data;
    stored_country_ = country_code;
    delta_compressed_seed_ = is_delta_compressed;
    gzip_compressed_seed_ = is_gzip_compressed;
    return true;
  }

 private:
  bool LoadSeed(VariationsSeed* seed) override {
    if (!seed_stored_)
      return false;
    return seed->ParseFromString(stored_seed_data_);
  }

  bool seed_stored_;
  std::string stored_seed_data_;
  std::string stored_country_;
  bool delta_compressed_seed_;
  bool gzip_compressed_seed_;

  DISALLOW_COPY_AND_ASSIGN(TestVariationsFieldTrialCreator);
};

// Constants used to create the test seed.
const char kTestSeedStudyName[] = "test";
const char kTestSeedExperimentName[] = "abc";
const int kTestSeedExperimentProbability = 100;
const char kTestSeedSerialNumber[] = "123";

// Populates |seed| with simple test data. The resulting seed will contain one
// study called "test", which contains one experiment called "abc" with
// probability weight 100. |seed|'s study field will be cleared before adding
// the new study.
VariationsSeed CreateTestSeed() {
  VariationsSeed seed;
  Study* study = seed.add_study();
  study->set_name(kTestSeedStudyName);
  study->set_default_experiment_name(kTestSeedExperimentName);
  Study_Experiment* experiment = study->add_experiment();
  experiment->set_name(kTestSeedExperimentName);
  experiment->set_probability_weight(kTestSeedExperimentProbability);
  seed.set_serial_number(kTestSeedSerialNumber);
  return seed;
}

// Serializes |seed| to protobuf binary format.
std::string SerializeSeed(const VariationsSeed& seed) {
  std::string serialized_seed;
  seed.SerializeToString(&serialized_seed);
  return serialized_seed;
}

}  // namespace

class FieldTrialCreatorTest : public ::testing::Test {
 protected:
  FieldTrialCreatorTest() {}

 private:
  base::MessageLoop message_loop_;

  DISALLOW_COPY_AND_ASSIGN(FieldTrialCreatorTest);
};

TEST_F(FieldTrialCreatorTest, CreateTrialsFromSeed) {
  TestingPrefServiceSimple prefs;
  VariationsService::RegisterPrefs(prefs.registry());

  // Create a local base::FieldTrialList, to hold the field trials created in
  // this test.
  base::FieldTrialList field_trial_list(nullptr);

  // Create a variations service.
  TestVariationsFieldTrialCreator field_trial_creator(
      base::MakeUnique<web_resource::TestRequestAllowedNotifier>(&prefs),
      &prefs);
  field_trial_creator.SetCreateTrialsFromSeedCalledForTesting(false);

  // Store a seed.
  field_trial_creator.StoreSeed(SerializeSeed(CreateTestSeed()), std::string(),
                                std::string(), base::Time::Now(), false, false);
  prefs.SetInt64(prefs::kVariationsLastFetchTime,
                 base::Time::Now().ToInternalValue());

  // Check that field trials are created from the seed. Since the test study has
  // only 1 experiment with 100% probability weight, we must be part of it.
  EXPECT_TRUE(field_trial_creator.CreateTrialsFromSeed(
      base::FeatureList::GetInstance(),
      std::unique_ptr<const base::FieldTrial::EntropyProvider>(nullptr)));
  EXPECT_EQ(kTestSeedExperimentName,
            base::FieldTrialList::FindFullName(kTestSeedStudyName));
}

TEST_F(FieldTrialCreatorTest, CreateTrialsFromSeedNoLastFetchTime) {
  TestingPrefServiceSimple prefs;
  VariationsService::RegisterPrefs(prefs.registry());

  // Create a local base::FieldTrialList, to hold the field trials created in
  // this test.
  base::FieldTrialList field_trial_list(nullptr);

  // Create a variations service
  TestVariationsFieldTrialCreator field_trial_creator(
      base::MakeUnique<web_resource::TestRequestAllowedNotifier>(&prefs),
      &prefs);
  field_trial_creator.SetCreateTrialsFromSeedCalledForTesting(false);

  // Store a seed. To simulate a first run, |prefs::kVariationsLastFetchTime|
  // is left empty.
  field_trial_creator.StoreSeed(SerializeSeed(CreateTestSeed()), std::string(),
                                std::string(), base::Time::Now(), false, false);
  EXPECT_EQ(0, prefs.GetInt64(prefs::kVariationsLastFetchTime));

  // Check that field trials are created from the seed. Since the test study has
  // only 1 experiment with 100% probability weight, we must be part of it.
  EXPECT_TRUE(field_trial_creator.CreateTrialsFromSeed(
      base::FeatureList::GetInstance(),
      std::unique_ptr<const base::FieldTrial::EntropyProvider>(nullptr)));
  EXPECT_EQ(base::FieldTrialList::FindFullName(kTestSeedStudyName),
            kTestSeedExperimentName);
}

TEST_F(FieldTrialCreatorTest, CreateTrialsFromOutdatedSeed) {
  TestingPrefServiceSimple prefs;
  VariationsService::RegisterPrefs(prefs.registry());

  // Create a local base::FieldTrialList, to hold the field trials created in
  // this test.
  base::FieldTrialList field_trial_list(nullptr);

  // Create a variations service.
  TestVariationsFieldTrialCreator field_trial_creator(
      base::MakeUnique<web_resource::TestRequestAllowedNotifier>(&prefs),
      &prefs);
  field_trial_creator.SetCreateTrialsFromSeedCalledForTesting(false);

  // Store a seed, with a fetch time 31 days in the past.
  const base::Time seed_date =
      base::Time::Now() - base::TimeDelta::FromDays(31);
  field_trial_creator.StoreSeed(SerializeSeed(CreateTestSeed()), std::string(),
                                std::string(), seed_date, false, false);
  prefs.SetInt64(prefs::kVariationsLastFetchTime, seed_date.ToInternalValue());

  // Check that field trials are not created from the seed.
  EXPECT_FALSE(field_trial_creator.CreateTrialsFromSeed(
      base::FeatureList::GetInstance(),
      std::unique_ptr<const base::FieldTrial::EntropyProvider>(nullptr)));
  EXPECT_TRUE(base::FieldTrialList::FindFullName(kTestSeedStudyName).empty());
}

}  // namespace variations
