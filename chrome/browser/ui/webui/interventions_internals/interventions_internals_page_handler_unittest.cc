// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/interventions_internals/interventions_internals_page_handler.h"

#include <unordered_map>

#include "base/macros.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_task_environment.h"
#include "components/previews/core/previews_features.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// The key for the mapping for the enabled/disabled status map.
const char kAMPRedirectionPreviews[] = "AMP Previews";
const char kClientLoFiPreviews[] = "Client LoFi Previews";
const char kOfflinePreviews[] = "Offline Previews";

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

    scoped_feature_list_ = base::MakeUnique<base::test::ScopedFeatureList>();

    // Clear testing map.
    passedInParams.clear();
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  mojom::InterventionsInternalsPageHandlerPtr pageHandlerPtr;
  mojom::InterventionsInternalsPageHandlerRequest request;
  std::unique_ptr<InterventionsInternalsPageHandler> pageHandler;
  std::unique_ptr<base::test::ScopedFeatureList> scoped_feature_list_;
};

TEST_F(InterventionsInternalsPageHandlerTest, CorrectSizeOfPassingParameters) {
  pageHandler->GetPreviewsEnabled(
      base::BindOnce(*MockGetPreviewsEnabledCallback));

  const unsigned long expected = 3;
  EXPECT_EQ(expected, passedInParams.size());
}

TEST_F(InterventionsInternalsPageHandlerTest, AMPRedirectionDisabled) {
  // Init with kAMPRedirection disabled.
  scoped_feature_list_->InitWithFeatures({},
                                         {previews::features::kAMPRedirection});

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(*MockGetPreviewsEnabledCallback));
  auto amp_redirection = passedInParams.find(kAMPRedirectionPreviews);
  ASSERT_NE(passedInParams.end(), amp_redirection);
  EXPECT_FALSE(amp_redirection->second);
}

TEST_F(InterventionsInternalsPageHandlerTest, AMPRedirectionEnabled) {
  // Init with kAMPRedirection enabled.
  scoped_feature_list_->InitWithFeatures({previews::features::kAMPRedirection},
                                         {});

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(*MockGetPreviewsEnabledCallback));
  auto amp_redirection = passedInParams.find(kAMPRedirectionPreviews);
  ASSERT_NE(passedInParams.end(), amp_redirection);
  EXPECT_TRUE(amp_redirection->second);
}

TEST_F(InterventionsInternalsPageHandlerTest, TestClientLoFiDisabled) {
  // Init with kClientLoFi disabled.
  scoped_feature_list_->InitWithFeatures({}, {previews::features::kClientLoFi});

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(*MockGetPreviewsEnabledCallback));
  auto client_lofi = passedInParams.find(kClientLoFiPreviews);
  ASSERT_NE(passedInParams.end(), client_lofi);
  EXPECT_FALSE(client_lofi->second);
}

TEST_F(InterventionsInternalsPageHandlerTest, TestClientLoFiEnabled) {
  // Init with kClientLoFi enabled.
  scoped_feature_list_->InitWithFeatures({previews::features::kClientLoFi}, {});

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(*MockGetPreviewsEnabledCallback));
  auto client_lofi = passedInParams.find(kClientLoFiPreviews);
  ASSERT_NE(passedInParams.end(), client_lofi);
  EXPECT_TRUE(client_lofi->second);
}

TEST_F(InterventionsInternalsPageHandlerTest, OfflinePreviewsDisabled) {
  // Init with kOfflinePreviews disabled.
  scoped_feature_list_->InitWithFeatures(
      {}, {previews::features::kOfflinePreviews});

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(*MockGetPreviewsEnabledCallback));
  auto offline_previews = passedInParams.find(kOfflinePreviews);
  ASSERT_NE(passedInParams.end(), offline_previews);
  EXPECT_FALSE(offline_previews->second);
}

TEST_F(InterventionsInternalsPageHandlerTest, OfflinePreviewsEnabled) {
  // Init with kOfflinePreviews enabled.
  scoped_feature_list_->InitWithFeatures({previews::features::kOfflinePreviews},
                                         {});

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(*MockGetPreviewsEnabledCallback));
  auto offline_previews = passedInParams.find(kOfflinePreviews);
  ASSERT_NE(passedInParams.end(), offline_previews);
  EXPECT_TRUE(offline_previews->second);
}
