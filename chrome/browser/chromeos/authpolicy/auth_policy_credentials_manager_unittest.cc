// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/authpolicy/auth_policy_credentials_manager.h"

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/login/users/fake_chrome_user_manager.h"
#include "chrome/browser/chromeos/login/users/mock_user_manager.h"
#include "chrome/browser/chromeos/login/users/scoped_user_manager_enabler.h"
#include "chrome/browser/notifications/notification_test_util.h"
#include "chrome/browser/notifications/notification_ui_manager.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_auth_policy_client.h"
#include "chromeos/network/network_handler.h"
#include "components/user_manager/user_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace {

const char kProfileSigninNotificationId[] = "chrome://settings/signin/";
const char kDisplayName[] = "DisplayName";
const char kGivenName[] = "Given Name";

MATCHER_P(UserAccountDataEq, value, "Compares two UserAccountData") {
  const user_manager::UserManager::UserAccountData& expected_data = value;
  return expected_data.display_name() == arg.display_name() &&
         expected_data.given_name() == arg.given_name() &&
         expected_data.locale() == arg.locale();
}

}  // namespace

class AuthPolicyCredentialsManagerTest : public testing::Test {
 public:
  AuthPolicyCredentialsManagerTest() = default;
  ~AuthPolicyCredentialsManagerTest() override = default;

  void SetUp() override {
    chromeos::DBusThreadManager::Initialize();
    chromeos::NetworkHandler::Initialize();
    user_manager_enabler_ =
        base::MakeUnique<ScopedUserManagerEnabler>(new FakeChromeUserManager());
    fake_auth_policy_client()->set_operation_delay(
        base::TimeDelta::FromSeconds(0));

    TestingBrowserProcess::GetGlobal()->SetNotificationUIManager(
        base::MakeUnique<StubNotificationUIManager>());

    TestingProfile::Builder profile_builder;
    profile_builder.SetProfileName("user@gmail.com");
    profile_ = profile_builder.Build();
    account_id_ = AccountId::AdFromUserEmailObjGuid(
        profile()->GetProfileUserName(), "1234567890");
    fake_user_manager()->AddUser(account_id_);

    run_loop_ = base::MakeUnique<base::RunLoop>();
    fake_auth_policy_client()->set_closure(run_loop_->QuitClosure());

    auth_policy_credentials_manager_ = static_cast<
        AuthPolicyCredentialsManager*>(
        AuthPolicyCredentialsManagerFactory::BuildForProfileIfActiveDirectory(
            profile()));

    run_loop_->Run();

    mock_user_manager_ = new MockUserManager();
    user_manager_enabler_.reset();
    user_manager_enabler_ =
        base::MakeUnique<ScopedUserManagerEnabler>(mock_user_manager_);
  }

  void TearDown() override {
    EXPECT_CALL(*mock_user_manager(), Shutdown());
    chromeos::NetworkHandler::Shutdown();
    chromeos::DBusThreadManager::Shutdown();
  }

 protected:
  AccountId& account_id() { return account_id_; }
  TestingProfile* profile() { return profile_.get(); }
  AuthPolicyCredentialsManager* auth_policy_credentials_manager() {
    return auth_policy_credentials_manager_;
  }
  chromeos::FakeChromeUserManager* fake_user_manager() const {
    return static_cast<chromeos::FakeChromeUserManager*>(
        user_manager::UserManager::Get());
  }
  chromeos::FakeAuthPolicyClient* fake_auth_policy_client() const {
    return static_cast<chromeos::FakeAuthPolicyClient*>(
        chromeos::DBusThreadManager::Get()->GetAuthPolicyClient());
  }
  NotificationUIManager* ui_manager() const {
    return TestingBrowserProcess::GetGlobal()->notification_ui_manager();
  }
  MockUserManager* mock_user_manager() { return mock_user_manager_; }
  void CallGetUserStatusAndWait() {
    run_loop_ = base::MakeUnique<base::RunLoop>();
    fake_auth_policy_client()->set_closure(run_loop_->QuitClosure());
    auth_policy_credentials_manager()->GetUserStatus();
    run_loop_->Run();
  }
  std::unique_ptr<base::RunLoop> run_loop_;

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  AccountId account_id_;
  std::unique_ptr<TestingProfile> profile_;
  AuthPolicyCredentialsManager* auth_policy_credentials_manager_;
  std::unique_ptr<chromeos::ScopedUserManagerEnabler> user_manager_enabler_;
  MockUserManager* mock_user_manager_;

  DISALLOW_COPY_AND_ASSIGN(AuthPolicyCredentialsManagerTest);
};

// Test notifications, setting display and given name, saving force online
// signing flag.
TEST_F(AuthPolicyCredentialsManagerTest, All) {
  // Test setting display and given name.
  fake_auth_policy_client()->set_display_name(kDisplayName);
  fake_auth_policy_client()->set_given_name(kGivenName);
  user_manager::UserManager::UserAccountData user_account_data(
      base::UTF8ToUTF16(kDisplayName), base::UTF8ToUTF16(kGivenName),
      std::string() /* locale */);

  EXPECT_CALL(*mock_user_manager(),
              UpdateUserAccountData(
                  account_id(),
                  UserAccountDataEq(::testing::ByRef(user_account_data))));
  EXPECT_CALL(*mock_user_manager(), SaveForceOnlineSignin(account_id(), false));
  CallGetUserStatusAndWait();
  // No notifications.
  EXPECT_EQ(0ul, ui_manager()->GetAllIdsByProfile(profile()).size());

  // In case of expired password save to force online signin and show
  // notification.
  fake_auth_policy_client()->set_password_status(
      authpolicy::ActiveDirectoryUserStatus::PASSWORD_EXPIRED);
  EXPECT_CALL(*mock_user_manager(), SaveForceOnlineSignin(account_id(), true));
  CallGetUserStatusAndWait();
  std::string notification_id =
      kProfileSigninNotificationId + profile()->GetProfileUserName() +
      std::to_string(IDS_ACTIVE_DIRECTORY_PASSWORD_EXPIRED);
  EXPECT_EQ(1ul, ui_manager()->GetAllIdsByProfile(profile()).size());
  EXPECT_TRUE(ui_manager()->CancelById(notification_id, profile()));

  // Do not show the same notification twice.
  EXPECT_CALL(*mock_user_manager(), SaveForceOnlineSignin(account_id(), true));
  CallGetUserStatusAndWait();
  EXPECT_EQ(0ul, ui_manager()->GetAllIdsByProfile(profile()).size());

  // In case of expired TGT save to force online signin and show
  // notification.
  fake_auth_policy_client()->set_tgt_status(
      authpolicy::ActiveDirectoryUserStatus::TGT_EXPIRED);
  EXPECT_CALL(*mock_user_manager(), SaveForceOnlineSignin(account_id(), true));
  CallGetUserStatusAndWait();
  notification_id = kProfileSigninNotificationId +
                    profile()->GetProfileUserName() +
                    std::to_string(IDS_ACTIVE_DIRECTORY_REFRESH_AUTH_TOKEN);

  EXPECT_EQ(1ul, ui_manager()->GetAllIdsByProfile(profile()).size());
  EXPECT_TRUE(ui_manager()->CancelById(notification_id, profile()));
  EXPECT_EQ(0ul, ui_manager()->GetAllIdsByProfile(profile()).size());
}

}  // namespace chromeos
