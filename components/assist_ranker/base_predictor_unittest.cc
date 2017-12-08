// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/assist_ranker/base_predictor.h"

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/test/scoped_feature_list.h"
#include "components/assist_ranker/fake_ranker_model_loader.h"
#include "components/assist_ranker/predictor_config.h"
#include "components/assist_ranker/ranker_model.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace assist_ranker {

using ::assist_ranker::testing::FakeRankerModelLoader;

namespace {

// Predictor config for testing.
const char kTestModelName[] = "test_model";
const char kTestLoggingName[] = "Test";
const char kTestUmaPrefixName[] = "Test.Ranker";
const char kTestUrlParamName[] = "ranker-model-url";
const char kTestDefaultModelUrl[] = "https://foo.bar/model.bin";

const base::Feature kTestRankerQuery{"TestRankerQuery",
                                     base::FEATURE_ENABLED_BY_DEFAULT};

const base::FeatureParam<std::string> kTestRankerUrl{
    &kTestRankerQuery, kTestUrlParamName, kTestDefaultModelUrl};

const PredictorConfig kTestPredictorConfig = PredictorConfig{
    kTestModelName,      kTestLoggingName,  kTestUmaPrefixName, LOG_UKM,
    GetEmptyWhitelist(), &kTestRankerQuery, &kTestRankerUrl};

// Class that implements virtual functions of the base class.
class FakePredictor : public BasePredictor {
 public:
  static std::unique_ptr<FakePredictor> Create();
  ~FakePredictor() override{};
  // Validation will always succeed.
  static RankerModelStatus ValidateModel(const RankerModel& model);

 protected:
  // Not implementing any inference logic.
  bool Initialize() override { return true; };

 private:
  FakePredictor(const PredictorConfig& config);
  DISALLOW_COPY_AND_ASSIGN(FakePredictor);
};

FakePredictor::FakePredictor(const PredictorConfig& config)
    : BasePredictor(config) {}

RankerModelStatus FakePredictor::ValidateModel(const RankerModel& model) {
  return RankerModelStatus::OK;
}

std::unique_ptr<FakePredictor> FakePredictor::Create() {
  std::unique_ptr<FakePredictor> predictor(
      new FakePredictor(kTestPredictorConfig));
  auto ranker_model = base::MakeUnique<RankerModel>();
  auto fake_model_loader = base::MakeUnique<FakeRankerModelLoader>(
      base::BindRepeating(&FakePredictor::ValidateModel),
      base::BindRepeating(&FakePredictor::OnModelAvailable,
                          base::Unretained(predictor.get())),
      std::move(ranker_model));
  predictor->LoadModel(std::move(fake_model_loader));
  return predictor;
}

}  // namespace

class BasePredictorTest : public ::testing::Test {
 protected:
  BasePredictorTest() = default;
  // Disables Query for the test predictor.
  void DisableQuery();

 private:
  // Manages the enabling/disabling of features within the scope of a test.
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(BasePredictorTest);
};

void BasePredictorTest::DisableQuery() {
  scoped_feature_list_.InitWithFeatures({}, {kTestRankerQuery});
}

TEST_F(BasePredictorTest, BaseTest) {
  auto predictor = FakePredictor::Create();
  EXPECT_EQ(kTestModelName, predictor->GetModelName());
  EXPECT_EQ(kTestDefaultModelUrl, predictor->GetModelUrl());
  EXPECT_TRUE(predictor->is_query_enabled());
  EXPECT_TRUE(predictor->IsReady());
}

TEST_F(BasePredictorTest, QueryDisabled) {
  DisableQuery();
  auto predictor = FakePredictor::Create();
  EXPECT_EQ(kTestModelName, predictor->GetModelName());
  EXPECT_EQ(kTestDefaultModelUrl, predictor->GetModelUrl());
  EXPECT_FALSE(predictor->is_query_enabled());
  EXPECT_FALSE(predictor->IsReady());
}

// FIXME: Add tests for UKM logging.

}  // namespace assist_ranker
