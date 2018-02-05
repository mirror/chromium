// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/remote/remote_suggestions_status_service_impl.h"

#include <memory>

#include "build/build_config.h"
#include "components/ntp_snippets/features.h"
#include "components/ntp_snippets/ntp_snippets_constants.h"
#include "components/ntp_snippets/pref_names.h"
#include "components/ntp_snippets/remote/test_utils.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/variations/variations_params_manager.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_snippets {

namespace {

void OnStatusChange(RemoteSuggestionsStatus old_status,
                    RemoteSuggestionsStatus new_status) {}

}  // namespace

class RemoteSuggestionsStatusServiceImplTest : public ::testing::Test {
 public:
  RemoteSuggestionsStatusServiceImplTest() {
    RemoteSuggestionsStatusServiceImpl::RegisterProfilePrefs(
        utils_.pref_service()->registry());
  }

  std::unique_ptr<RemoteSuggestionsStatusServiceImpl> MakeService() {
    auto service = std::make_unique<RemoteSuggestionsStatusServiceImpl>(
        false, utils_.pref_service());
    service->Init(base::BindRepeating(&OnStatusChange));
    return service;
  }

 protected:
  test::RemoteSuggestionsTestUtils utils_;
  variations::testing::VariationParamsManager params_manager_;
};

TEST_F(RemoteSuggestionsStatusServiceImplTest, NoSigninNeeded) {
  auto service = MakeService();

  // By default, no signin is required.
  EXPECT_EQ(RemoteSuggestionsStatus::ENABLED_AND_SIGNED_OUT,
            service->GetStatusFromDeps());

  // Signin should cause a state change.
  service->OnSignInStateChanged(/*has_signed_in=*/true);
  EXPECT_EQ(RemoteSuggestionsStatus::ENABLED_AND_SIGNED_IN,
            service->GetStatusFromDeps());
}

TEST_F(RemoteSuggestionsStatusServiceImplTest, DisabledViaPref) {
  auto service = MakeService();

  // The default test setup is signed out. The service is enabled.
  ASSERT_EQ(RemoteSuggestionsStatus::ENABLED_AND_SIGNED_OUT,
            service->GetStatusFromDeps());

  // Once the enabled pref is set to false, we should be disabled.
  utils_.pref_service()->SetBoolean(prefs::kEnableSnippets, false);
  EXPECT_EQ(RemoteSuggestionsStatus::EXPLICITLY_DISABLED,
            service->GetStatusFromDeps());

  // The state should not change, even though a signin has occurred.
  service->OnSignInStateChanged(/*has_signed_in=*/true);
  EXPECT_EQ(RemoteSuggestionsStatus::EXPLICITLY_DISABLED,
            service->GetStatusFromDeps());
}

TEST_F(RemoteSuggestionsStatusServiceImplTest, EnabledAfterListFolded) {
  auto service = MakeService();

  // The default test setup is signed out. The service is enabled.
  ASSERT_EQ(RemoteSuggestionsStatus::ENABLED_AND_SIGNED_OUT,
            service->GetStatusFromDeps());
  // By default, the articles list should be visible.
  EXPECT_TRUE(utils_.pref_service()->GetBoolean(prefs::kArticlesListVisible));

  // Once the user toggles the visibility of articles list in UI off, the
  // preference should be set appropriately, but the service should still be
  // enabled.
  service->OnListVisibilityToggled(/*visible=*/false);
  EXPECT_EQ(RemoteSuggestionsStatus::ENABLED_AND_SIGNED_OUT,
            service->GetStatusFromDeps());
  EXPECT_FALSE(utils_.pref_service()->GetBoolean(prefs::kArticlesListVisible));

  // Signin should cause a state change.
  service->OnSignInStateChanged(/*has_signed_in=*/true);
  EXPECT_EQ(RemoteSuggestionsStatus::ENABLED_AND_SIGNED_IN,
            service->GetStatusFromDeps());
}

TEST_F(RemoteSuggestionsStatusServiceImplTest, DisabledWhenListFoldedOnStart) {
  utils_.pref_service()->SetBoolean(prefs::kArticlesListVisible, false);
  auto service = MakeService();

  // The state should be disabled when starting with no list shown.
  EXPECT_EQ(RemoteSuggestionsStatus::EXPLICITLY_DISABLED,
            service->GetStatusFromDeps());
  // Service initialization is not meant to change the setting, only to read it.
  EXPECT_FALSE(utils_.pref_service()->GetBoolean(prefs::kArticlesListVisible));

  // The state should not change, even though a signin has occurred.
  service->OnSignInStateChanged(/*has_signed_in=*/true);
  EXPECT_EQ(RemoteSuggestionsStatus::EXPLICITLY_DISABLED,
            service->GetStatusFromDeps());
}

TEST_F(RemoteSuggestionsStatusServiceImplTest, EnablingAfterFoldedStart) {
  utils_.pref_service()->SetBoolean(prefs::kArticlesListVisible, false);
  auto service = MakeService();

  // Once the user toggles the visibility of articles list in UI on, the
  // preference should be set appropriately and the service should get enabled.
  service->OnListVisibilityToggled(/*visible=*/true);
  EXPECT_EQ(RemoteSuggestionsStatus::ENABLED_AND_SIGNED_OUT,
            service->GetStatusFromDeps());
  EXPECT_TRUE(utils_.pref_service()->GetBoolean(prefs::kArticlesListVisible));

  // Signin should cause a state change.
  service->OnSignInStateChanged(/*has_signed_in=*/true);
  EXPECT_EQ(RemoteSuggestionsStatus::ENABLED_AND_SIGNED_IN,
            service->GetStatusFromDeps());
}

TEST_F(RemoteSuggestionsStatusServiceImplTest,
       EnablingAfterFoldedStartSignedIn) {
  utils_.pref_service()->SetBoolean(prefs::kArticlesListVisible, false);
  auto service = MakeService();

  // Signin should not cause a state change, because UI is not visible.
  service->OnSignInStateChanged(/*has_signed_in=*/true);
  EXPECT_EQ(RemoteSuggestionsStatus::EXPLICITLY_DISABLED,
            service->GetStatusFromDeps());

  // Once the user toggles the visibility of articles list in UI on, the
  // preference should be set appropriately and the service should get enabled.
  service->OnListVisibilityToggled(/*visible=*/true);
  EXPECT_EQ(RemoteSuggestionsStatus::ENABLED_AND_SIGNED_IN,
            service->GetStatusFromDeps());
  EXPECT_TRUE(utils_.pref_service()->GetBoolean(prefs::kArticlesListVisible));
}

}  // namespace ntp_snippets
