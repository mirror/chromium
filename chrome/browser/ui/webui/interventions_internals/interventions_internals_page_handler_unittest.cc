// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/interventions_internals/interventions_internals_page_handler.h"

#include "base/feature_list.h"
#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/test/scoped_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kOfflinePreviews[] = "offlinePreviews";
const char kClientLoFi[] = "clientLoFi";
const char kClientLoFiFieldTrial[] = "PreviewsClientLoFi";

std::unordered_map<std::string, bool> passedInParams;

void MockGetPreviewsEnabledCallback(
    const std::unordered_map<std::string, bool>& params) {
  passedInParams = params;
}

}  // namespace

class InterventionsInternalsPageHandlerTest : public testing::Test {
 public:
  InterventionsInternalsPageHandlerTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO) {}

  ~InterventionsInternalsPageHandlerTest() override {}

  void SetUp() override {
    scoped_task_environment_.RunUntilIdle();

    request = mojo::MakeRequest(&pageHandlerPtr);
    pageHandler.reset(
        new InterventionsInternalsPageHandler(std::move(request)));

    feature_list = base::MakeUnique<base::FeatureList>();

    // Clear testing map.
    passedInParams.clear();
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  mojom::InterventionsInternalsPageHandlerPtr pageHandlerPtr;
  mojom::InterventionsInternalsPageHandlerRequest request;
  std::unique_ptr<InterventionsInternalsPageHandler> pageHandler;
  std::unique_ptr<base::FeatureList> feature_list;
};

TEST_F(InterventionsInternalsPageHandlerTest, CorrectSizeOfPassingParameters) {
  pageHandler->GetPreviewsEnabled(
      base::BindOnce(*MockGetPreviewsEnabledCallback));

  const unsigned long expected = 3;
  EXPECT_EQ(expected, passedInParams.size());
}

TEST_F(InterventionsInternalsPageHandlerTest, TestClientLoFiDisabledByDefault) {
  base::FieldTrialList field_trial_list(nullptr);

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(*MockGetPreviewsEnabledCallback));
  EXPECT_FALSE(passedInParams[kClientLoFi]);
}

TEST_F(InterventionsInternalsPageHandlerTest,
       TestClientLoFiExplicitlyDisabled) {
  base::FieldTrialList field_trial_list(nullptr);

  EXPECT_TRUE(base::FieldTrialList::CreateFieldTrial(kClientLoFiFieldTrial,
                                                     "Disabled"));

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(*MockGetPreviewsEnabledCallback));
  EXPECT_FALSE(passedInParams[kClientLoFi]);
}

TEST_F(InterventionsInternalsPageHandlerTest, TestClientLoFiEnabled) {
  base::FieldTrialList field_trial_list(nullptr);

  EXPECT_TRUE(
      base::FieldTrialList::CreateFieldTrial(kClientLoFiFieldTrial, "Enabled"));

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(*MockGetPreviewsEnabledCallback));
  EXPECT_TRUE(passedInParams[kClientLoFi]);
}

TEST_F(InterventionsInternalsPageHandlerTest, OfflinePreviewsDefault) {
  pageHandler->GetPreviewsEnabled(
      base::BindOnce(*MockGetPreviewsEnabledCallback));
  EXPECT_TRUE(passedInParams[kOfflinePreviews]);
}

TEST_F(InterventionsInternalsPageHandlerTest, OfflinePreviewsDisabled) {
  feature_list->InitializeFromCommandLine("", "OfflinePreviews");
  base::FeatureList::ClearInstanceForTesting();
  base::FeatureList::SetInstance(std::move(feature_list));

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(*MockGetPreviewsEnabledCallback));
  EXPECT_FALSE(passedInParams[kOfflinePreviews]);
  base::FeatureList::ClearInstanceForTesting();
}
