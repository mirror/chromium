// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/signin/dice_turn_sync_on_helper.h"

#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "chrome/browser/signin/chrome_signin_client_factory.h"
#include "chrome/browser/signin/fake_profile_oauth2_token_service_builder.h"
#include "chrome/browser/signin/fake_signin_manager_builder.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/signin/test_signin_client_builder.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/scoped_account_consistency.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

class DiceTurnSyncOnHelperTest;

namespace {

const char kEmail[] = "email@foo.com";
const char kPreviousEmail[] = "notme@bar.com";
const char kGaiaID[] = "gaia_id";
const signin_metrics::AccessPoint kAccessPoint =
    signin_metrics::AccessPoint::ACCESS_POINT_BOOKMARK_MANAGER;
const signin_metrics::Reason kSigninReason =
    signin_metrics::Reason::REASON_REAUTHENTICATION;

// Dummy delegate forwarding all the calls the test fixture.
// Owned by the DiceTurnOnSyncHelper.
class TestDiceTurnSyncOnHelperDelegate : public DiceTurnSyncOnHelper::Delegate {
 public:
  TestDiceTurnSyncOnHelperDelegate(DiceTurnSyncOnHelperTest* test_fixture);
  ~TestDiceTurnSyncOnHelperDelegate() override;

 private:
  // DiceTurnSyncOnHelper::Delegate:
  void ShowLoginError(const std::string& email,
                      const std::string& error_message) override;
  void ShowMergeSyncDataConfirmation(
      const std::string& previous_email,
      const std::string& new_email,
      DiceTurnSyncOnHelper::SigninChoiceCallback callback) override;
  void ShowEnterpriseAccountConfirmation(
      const std::string& email,
      DiceTurnSyncOnHelper::SigninChoiceCallback callback) override;
  void ShowSyncConfirmation(
      base::OnceCallback<void(LoginUIService::SyncConfirmationUIClosedResult)>
          callback) override;
  void ShowSyncSettings() override;
  void ShowSigninPageInNewProfile(Profile* new_profile,
                                  const std::string& username) override;

  DiceTurnSyncOnHelperTest* test_fixture_;
};

}  // namespace

class DiceTurnSyncOnHelperTest : public testing::Test {
 public:
  DiceTurnSyncOnHelperTest() {
    TestingProfile::Builder profile_builder;
    profile_builder.AddTestingFactory(
        ProfileOAuth2TokenServiceFactory::GetInstance(),
        BuildFakeProfileOAuth2TokenService);
    profile_builder.AddTestingFactory(SigninManagerFactory::GetInstance(),
                                      BuildFakeSigninManagerBase);
    profile_builder.AddTestingFactory(ChromeSigninClientFactory::GetInstance(),
                                      signin::BuildTestSigninClient);
    profile_ = profile_builder.Build();

    account_id_ =
        AccountTrackerServiceFactory::GetForProfile(profile())->SeedAccountInfo(
            kGaiaID, kEmail);
    token_service_ = ProfileOAuth2TokenServiceFactory::GetForProfile(profile());
    token_service_->UpdateCredentials(account_id_, "refresh_token");
    EXPECT_TRUE(token_service_->RefreshTokenIsAvailable(account_id_));
  }

  ~DiceTurnSyncOnHelperTest() override {
    DCHECK(delegate_destroyed_);
    CheckDelegateCalls();
  }

  // Basic accessors.
  Profile* profile() { return profile_.get(); }
  ProfileOAuth2TokenService* token_service() { return token_service_; }
  const std::string& account_id() { return account_id_; }

  DiceTurnSyncOnHelper* CreateDiceTurnOnSyncHelper(
      DiceTurnSyncOnHelper::SigninAbortedMode mode) {
    return new DiceTurnSyncOnHelper(
        profile(), kAccessPoint, kSigninReason, account_id_, mode,
        std::make_unique<TestDiceTurnSyncOnHelperDelegate>(this));
  }

  void CheckDelegateCalls() {
    EXPECT_EQ(expected_login_error_email_, login_error_email_);
    EXPECT_EQ(expected_login_error_message_, login_error_message_);
  }

  // Functions called by the DiceTurnSyncOnHelper::Delegate:
  void OnShowLoginError(const std::string& email,
                        const std::string& error_message) {
    EXPECT_FALSE(sync_confirmation_shown_);
    EXPECT_FALSE(email.empty());
    EXPECT_TRUE(login_error_email_.empty())
        << "Login error should be shown only once.";
    login_error_email_ = email;
    login_error_message_ = error_message;  // May be empty.
  }

  void OnShowMergeSyncDataConfirmation(
      const std::string& previous_email,
      const std::string& new_email,
      DiceTurnSyncOnHelper::SigninChoiceCallback callback) {
    EXPECT_FALSE(sync_confirmation_shown_);
    EXPECT_FALSE(previous_email.empty());
    EXPECT_FALSE(new_email.empty());
    EXPECT_TRUE(merge_data_previous_email_.empty())
        << "Merge data confirmation should be shown only once";
    EXPECT_TRUE(merge_data_new_email_.empty())
        << "Merge data confirmation should be shown only once";
    merge_data_previous_email_ = previous_email;
    merge_data_new_email_ = new_email;
    std::move(callback).Run(merge_data_choice_);
  }

  void OnShowEnterpriseAccountConfirmation(
      const std::string& email,
      DiceTurnSyncOnHelper::SigninChoiceCallback callback) {}

  void OnShowSyncConfirmation(
      base::OnceCallback<void(LoginUIService::SyncConfirmationUIClosedResult)>
          callback) {
    EXPECT_FALSE(sync_confirmation_shown_)
        << "Sync confirmation should be shown only once.";
    sync_confirmation_shown_ = true;
    std::move(callback).Run(sync_confirmation_result_);
  }

  void OnShowSyncSettings() {}

  void OnShowSigninPageInNewProfile(Profile* new_profile,
                                    const std::string& username) {}

  void OnDelegateDestroyed() { delegate_destroyed_ = true; }

 protected:
  // Delegate behavior.
  DiceTurnSyncOnHelper::SigninChoice merge_data_choice_ =
      DiceTurnSyncOnHelper::SIGNIN_CHOICE_CANCEL;
  LoginUIService::SyncConfirmationUIClosedResult sync_confirmation_result_ =
      LoginUIService::SyncConfirmationUIClosedResult::ABORT_SIGNIN;

  // Expected delegate calls.
  std::string expected_login_error_email_;
  std::string expected_login_error_message_;
  std::string expected_merge_data_previous_email_;
  std::string expected_merge_data_new_email_;
  bool expected_sync_confirmation_shown_ = false;

 private:
  signin::ScopedAccountConsistencyDicePrepareMigration scoped_dice;
  content::TestBrowserThreadBundle thread_bundle_;
  std::string account_id_;
  std::unique_ptr<TestingProfile> profile_;
  ProfileOAuth2TokenService* token_service_ = nullptr;

  // State of the delegate calls.
  bool delegate_destroyed_ = false;
  std::string login_error_email_;
  std::string login_error_message_;
  std::string merge_data_previous_email_;
  std::string merge_data_new_email_;
  bool sync_confirmation_shown_ = false;
};

// TestDiceTurnSyncOnHelperDelegate implementation.

TestDiceTurnSyncOnHelperDelegate::TestDiceTurnSyncOnHelperDelegate(
    DiceTurnSyncOnHelperTest* test_fixture)
    : test_fixture_(test_fixture) {}

TestDiceTurnSyncOnHelperDelegate::~TestDiceTurnSyncOnHelperDelegate() {
  test_fixture_->OnDelegateDestroyed();
}

void TestDiceTurnSyncOnHelperDelegate::ShowLoginError(
    const std::string& email,
    const std::string& error_message) {
  test_fixture_->OnShowLoginError(email, error_message);
}

void TestDiceTurnSyncOnHelperDelegate::ShowMergeSyncDataConfirmation(
    const std::string& previous_email,
    const std::string& new_email,
    DiceTurnSyncOnHelper::SigninChoiceCallback callback) {
  test_fixture_->OnShowMergeSyncDataConfirmation(previous_email, new_email,
                                                 std::move(callback));
}

void TestDiceTurnSyncOnHelperDelegate::ShowEnterpriseAccountConfirmation(
    const std::string& email,
    DiceTurnSyncOnHelper::SigninChoiceCallback callback) {
  test_fixture_->OnShowEnterpriseAccountConfirmation(email,
                                                     std::move(callback));
}

void TestDiceTurnSyncOnHelperDelegate::ShowSyncConfirmation(
    base::OnceCallback<void(LoginUIService::SyncConfirmationUIClosedResult)>
        callback) {
  test_fixture_->OnShowSyncConfirmation(std::move(callback));
}

void TestDiceTurnSyncOnHelperDelegate::ShowSyncSettings() {
  test_fixture_->OnShowSyncSettings();
}

void TestDiceTurnSyncOnHelperDelegate::ShowSigninPageInNewProfile(
    Profile* new_profile,
    const std::string& username) {
  test_fixture_->OnShowSigninPageInNewProfile(new_profile, username);
}

TEST_F(DiceTurnSyncOnHelperTest, CanOfferSigninErrorKeepAccount) {
  expected_login_error_email_ = kEmail;
  profile()->GetPrefs()->SetBoolean(prefs::kSigninAllowed, false);
  CreateDiceTurnOnSyncHelper(
      DiceTurnSyncOnHelper::SigninAbortedMode::KEEP_ACCOUNT);
  EXPECT_TRUE(token_service()->RefreshTokenIsAvailable(account_id()));
}

TEST_F(DiceTurnSyncOnHelperTest, CanOfferSigninErrorRemoveAccount) {
  expected_login_error_email_ = kEmail;
  profile()->GetPrefs()->SetBoolean(prefs::kSigninAllowed, false);
  CreateDiceTurnOnSyncHelper(
      DiceTurnSyncOnHelper::SigninAbortedMode::REMOVE_ACCOUNT);
  EXPECT_FALSE(token_service()->RefreshTokenIsAvailable(account_id()));
}

TEST_F(DiceTurnSyncOnHelperTest, CrossAccountAbort) {
  expected_merge_data_previous_email_ = kPreviousEmail;
  expected_merge_data_new_email_ = kEmail;
  profile()->GetPrefs()->SetString(prefs::kGoogleServicesLastUsername,
                                   kPreviousEmail);
  CreateDiceTurnOnSyncHelper(
      DiceTurnSyncOnHelper::SigninAbortedMode::REMOVE_ACCOUNT);
  EXPECT_FALSE(token_service()->RefreshTokenIsAvailable(account_id()));
}

TEST_F(DiceTurnSyncOnHelperTest, CrossAccountContinue) {
  expected_merge_data_previous_email_ = kPreviousEmail;
  expected_merge_data_new_email_ = kEmail;
  expected_sync_confirmation_shown_ = true;
  merge_data_choice_ = DiceTurnSyncOnHelper::SIGNIN_CHOICE_CONTINUE;
  profile()->GetPrefs()->SetString(prefs::kGoogleServicesLastUsername,
                                   kPreviousEmail);
  CreateDiceTurnOnSyncHelper(
      DiceTurnSyncOnHelper::SigninAbortedMode::REMOVE_ACCOUNT);
  EXPECT_FALSE(token_service()->RefreshTokenIsAvailable(account_id()));
}
