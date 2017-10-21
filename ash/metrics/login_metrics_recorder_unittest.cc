// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/metrics/login_metrics_recorder.h"

#include "ash/login/mock_lock_screen_client.h"
#include "ash/login/ui/lock_contents_view.h"
#include "ash/login/ui/lock_screen.h"
#include "ash/login/ui/login_auth_user_view.h"
#include "ash/login/ui/login_test_base.h"
#include "ash/login/ui/login_test_utils.h"
#include "ash/shell.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/test/histogram_tester.h"
#include "ui/events/test/event_generator.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace {

constexpr char kAuthMethodUsageAsTabletHistogramName[] =
    "Ash.Login.Lock.AuthMethod.Used.TabletMode";
constexpr char kAuthMethodUsageAsClamShellHistogramName[] =
    "Ash.Login.Lock.AuthMethod.Used.ClamShellMode";
constexpr char kNumAttemptTilSuccessHistogramName[] =
    "Ash.Login.Lock.NumPasswordAttempts.UntilSuccess";
constexpr char kNumAttemptTilFailureHistogramName[] =
    "Ash.Login.Lock.NumPasswordAttempts.UntilFailure";
constexpr char kUserClicksHistogramName[] = "Ash.Login.Lock.UserClicks";
constexpr char kAuthMethodSwitchHistogramName[] =
    "Ash.Login.Lock.AuthMethod.Switched";

// Test fixture for the LoginMetricsRecorder class.
class LoginMetricsRecorderTest : public LoginTestBase {
 public:
  LoginMetricsRecorderTest() = default;
  ~LoginMetricsRecorderTest() override = default;

  // LoginTestBase:
  void SetUp() override {
    LoginTestBase::SetUp();
    LockScreen::Show();
    login_metrics_recorder_.reset(new LoginMetricsRecorder());
    histogram_tester_.reset(new base::HistogramTester());
  }

  void TearDown() override {
    login_metrics_recorder_.reset();
    LockScreen::Get()->Destroy();
    LoginTestBase::TearDown();
  }

 protected:
  void EnableTabletMode(bool enable) {
    Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(
        enable);
  }

  // The test target.
  std::unique_ptr<LoginMetricsRecorder> login_metrics_recorder_;

  // Used to verify recorded data.
  std::unique_ptr<base::HistogramTester> histogram_tester_;

 private:
  DISALLOW_COPY_AND_ASSIGN(LoginMetricsRecorderTest);
};

}  // namespace

// Verifies that different unlock attempts get recorded in UMA.
TEST_F(LoginMetricsRecorderTest, UnlockAttempts) {
  std::unique_ptr<MockLockScreenClient> client = BindMockLockScreenClient();
  client->set_authenticate_user_callback_result(false);
  auto* contents = new LockContentsView(mojom::TrayActionState::kNotAvailable,
                                        data_dispatcher());
  LockContentsView::TestApi test_api(contents);
  SetUserCount(1);
  std::unique_ptr<views::Widget> widget = CreateWidgetWithContent(contents);
  LoginAuthUserView::AuthMethods auth_method = LoginAuthUserView::AUTH_PASSWORD;
  AccountId primary_user =
      test_api.primary_auth()->current_user()->basic_user_info->account_id;
  EXPECT_TRUE(test_api.primary_auth()->auth_methods() == auth_method);
  EXPECT_CALL(*client,
              AuthenticateUser_(primary_user, testing::_, false, testing::_));
  EXPECT_CALL(*client, OnFocusPod(primary_user));

  // Authentication attempt with password "abc1";
  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.PressKey(ui::KeyboardCode::VKEY_A, 0);
  generator.PressKey(ui::KeyboardCode::VKEY_B, 0);
  generator.PressKey(ui::KeyboardCode::VKEY_C, 0);
  generator.PressKey(ui::KeyboardCode::VKEY_1, 0);
  generator.PressKey(ui::KeyboardCode::VKEY_RETURN, 0);

  base::RunLoop().RunUntilIdle();
  histogram_tester_->ExpectTotalCount(kAuthMethodUsageAsClamShellHistogramName,
                                      1);
  histogram_tester_->ExpectBucketCount(
      kAuthMethodUsageAsClamShellHistogramName,
      static_cast<int>(LoginMetricsRecorder::AuthMethod::kPassword), 1);

  // Authentication attempt with pin "1111"
  test_api.primary_auth()->SetAuthMethods(auth_method |
                                          LoginAuthUserView::AUTH_PIN);
  EXPECT_TRUE(test_api.primary_auth()->auth_methods() ==
              (auth_method | LoginAuthUserView::AUTH_PIN));
  EXPECT_CALL(*client,
              AuthenticateUser_(primary_user, testing::_, true, testing::_));
  generator.PressKey(ui::KeyboardCode::VKEY_1, 0);
  generator.PressKey(ui::KeyboardCode::VKEY_1, 0);
  generator.PressKey(ui::KeyboardCode::VKEY_1, 0);
  generator.PressKey(ui::KeyboardCode::VKEY_1, 0);
  generator.PressKey(ui::KeyboardCode::VKEY_RETURN, 0);

  base::RunLoop().RunUntilIdle();
  histogram_tester_->ExpectTotalCount(kAuthMethodUsageAsClamShellHistogramName,
                                      2);
  histogram_tester_->ExpectBucketCount(
      kAuthMethodUsageAsClamShellHistogramName,
      static_cast<int>(LoginMetricsRecorder::AuthMethod::kPin), 1);

  // Authentication attempt with easy unlock tap.
  test_api.primary_auth()->SetAuthMethods(auth_method |
                                          LoginAuthUserView::AUTH_TAP);
  EXPECT_TRUE(test_api.primary_auth()->auth_methods() ==
              (auth_method | LoginAuthUserView::AUTH_TAP));
  EXPECT_CALL(*client, AttemptUnlock(primary_user));
  generator.MoveMouseTo(MakeLoginPrimaryAuthTestApi(contents)
                            .user_view()
                            ->GetBoundsInScreen()
                            .CenterPoint());
  generator.ClickLeftButton();

  base::RunLoop().RunUntilIdle();
  histogram_tester_->ExpectTotalCount(kAuthMethodUsageAsClamShellHistogramName,
                                      3);
  histogram_tester_->ExpectBucketCount(
      kAuthMethodUsageAsClamShellHistogramName,
      static_cast<int>(LoginMetricsRecorder::AuthMethod::kSmartlock), 1);
}

// Verifies that click on the note action button is recorded correctly.
TEST_F(LoginMetricsRecorderTest, NoteActionButtonClick) {
  auto* contents = new LockContentsView(mojom::TrayActionState::kAvailable,
                                        data_dispatcher());
  SetUserCount(1);
  std::unique_ptr<views::Widget> widget = CreateWidgetWithContent(contents);

  LockContentsView::TestApi test_api(contents);
  EXPECT_TRUE(test_api.note_action()->visible());

  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.MoveMouseTo(
      test_api.note_action()->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();

  histogram_tester_->ExpectTotalCount(kUserClicksHistogramName, 1);
  histogram_tester_->ExpectBucketCount(
      kUserClicksHistogramName,
      static_cast<int>(LoginMetricsRecorder::LockScreenUserClickTarget::
                           kLockScreenNoteActionButton),
      1);
}

// Verifies that auth method usage is recorded correctly.
TEST_F(LoginMetricsRecorderTest, AuthMethodUsage) {
  EnableTabletMode(false);
  login_metrics_recorder_->SetAuthMethod(
      LoginMetricsRecorder::AuthMethod::kPassword);
  histogram_tester_->ExpectBucketCount(
      kAuthMethodUsageAsClamShellHistogramName,
      static_cast<int>(LoginMetricsRecorder::AuthMethod::kPassword), 1);
  histogram_tester_->ExpectTotalCount(kAuthMethodUsageAsTabletHistogramName, 0);

  login_metrics_recorder_->SetAuthMethod(
      LoginMetricsRecorder::AuthMethod::kPin);
  histogram_tester_->ExpectBucketCount(
      kAuthMethodUsageAsClamShellHistogramName,
      static_cast<int>(LoginMetricsRecorder::AuthMethod::kPin), 1);
  histogram_tester_->ExpectTotalCount(kAuthMethodUsageAsTabletHistogramName, 0);

  login_metrics_recorder_->SetAuthMethod(
      LoginMetricsRecorder::AuthMethod::kSmartlock);
  histogram_tester_->ExpectBucketCount(
      kAuthMethodUsageAsClamShellHistogramName,
      static_cast<int>(LoginMetricsRecorder::AuthMethod::kSmartlock), 1);
  histogram_tester_->ExpectTotalCount(kAuthMethodUsageAsTabletHistogramName, 0);
  histogram_tester_->ExpectTotalCount(kAuthMethodUsageAsClamShellHistogramName,
                                      3);

  EnableTabletMode(true);
  login_metrics_recorder_->SetAuthMethod(
      LoginMetricsRecorder::AuthMethod::kPassword);
  histogram_tester_->ExpectTotalCount(kAuthMethodUsageAsClamShellHistogramName,
                                      3);
  histogram_tester_->ExpectBucketCount(
      kAuthMethodUsageAsTabletHistogramName,
      static_cast<int>(LoginMetricsRecorder::AuthMethod::kPassword), 1);

  login_metrics_recorder_->SetAuthMethod(
      LoginMetricsRecorder::AuthMethod::kPin);
  histogram_tester_->ExpectTotalCount(kAuthMethodUsageAsClamShellHistogramName,
                                      3);
  histogram_tester_->ExpectBucketCount(
      kAuthMethodUsageAsTabletHistogramName,
      static_cast<int>(LoginMetricsRecorder::AuthMethod::kPin), 1);

  login_metrics_recorder_->SetAuthMethod(
      LoginMetricsRecorder::AuthMethod::kSmartlock);
  histogram_tester_->ExpectTotalCount(kAuthMethodUsageAsClamShellHistogramName,
                                      3);
  histogram_tester_->ExpectBucketCount(
      kAuthMethodUsageAsTabletHistogramName,
      static_cast<int>(LoginMetricsRecorder::AuthMethod::kSmartlock), 1);
}

// Verifies that auth method switching is recorded correctly.
TEST_F(LoginMetricsRecorderTest, AuthMethodSwitch) {
  login_metrics_recorder_->SetAuthMethod(
      LoginMetricsRecorder::AuthMethod::kPassword);
  histogram_tester_->ExpectTotalCount(kAuthMethodSwitchHistogramName, 0);

  // Switch from password to pin.
  login_metrics_recorder_->SetAuthMethod(
      LoginMetricsRecorder::AuthMethod::kPin);
  histogram_tester_->ExpectBucketCount(
      kAuthMethodSwitchHistogramName,
      static_cast<int>(
          LoginMetricsRecorder::AuthMethodSwitchType::kPasswordToPin),
      1);

  // Switch from pin to smart lock.
  login_metrics_recorder_->SetAuthMethod(
      LoginMetricsRecorder::AuthMethod::kSmartlock);
  histogram_tester_->ExpectBucketCount(
      kAuthMethodSwitchHistogramName,
      static_cast<int>(
          LoginMetricsRecorder::AuthMethodSwitchType::kPinToSmartlock),
      1);

  // Switch from smart lock to password.
  login_metrics_recorder_->SetAuthMethod(
      LoginMetricsRecorder::AuthMethod::kPassword);
  histogram_tester_->ExpectBucketCount(
      kAuthMethodSwitchHistogramName,
      static_cast<int>(
          LoginMetricsRecorder::AuthMethodSwitchType::kSmartlockToPassword),
      1);

  // Switch from password to smart lock.
  login_metrics_recorder_->SetAuthMethod(
      LoginMetricsRecorder::AuthMethod::kSmartlock);
  histogram_tester_->ExpectBucketCount(
      kAuthMethodSwitchHistogramName,
      static_cast<int>(
          LoginMetricsRecorder::AuthMethodSwitchType::kPasswordToSmartlock),
      1);

  // Switch from smart lock to pin.
  login_metrics_recorder_->SetAuthMethod(
      LoginMetricsRecorder::AuthMethod::kPin);
  histogram_tester_->ExpectBucketCount(
      kAuthMethodSwitchHistogramName,
      static_cast<int>(
          LoginMetricsRecorder::AuthMethodSwitchType::kSmartlockToPin),
      1);

  // Switch from pin to password.
  login_metrics_recorder_->SetAuthMethod(
      LoginMetricsRecorder::AuthMethod::kPassword);
  histogram_tester_->ExpectBucketCount(
      kAuthMethodSwitchHistogramName,
      static_cast<int>(
          LoginMetricsRecorder::AuthMethodSwitchType::kPinToPassword),
      1);
}

// Verifies that number of auth attempt is recorded correctly.
TEST_F(LoginMetricsRecorderTest, RecordNumLoginAttempts) {
  login_metrics_recorder_->RecordNumLoginAttempts(5, true /*success*/);
  histogram_tester_->ExpectTotalCount(kNumAttemptTilSuccessHistogramName, 1);
  histogram_tester_->ExpectBucketCount(kNumAttemptTilSuccessHistogramName, 5,
                                       1);

  login_metrics_recorder_->RecordNumLoginAttempts(7, false /*success*/);
  histogram_tester_->ExpectTotalCount(kNumAttemptTilSuccessHistogramName, 1);
  histogram_tester_->ExpectBucketCount(kNumAttemptTilSuccessHistogramName, 5,
                                       1);
  histogram_tester_->ExpectTotalCount(kNumAttemptTilFailureHistogramName, 1);
  histogram_tester_->ExpectBucketCount(kNumAttemptTilFailureHistogramName, 7,
                                       1);
}

// Verifies that user click evens on the lock screen is recorded correctly.
TEST_F(LoginMetricsRecorderTest, RecordUserClickEventOnLockScreen) {
  histogram_tester_->ExpectTotalCount(kUserClicksHistogramName, 0);
  login_metrics_recorder_->RecordUserClickEventOnLockScreen(
      LoginMetricsRecorder::LockScreenUserClickTarget::kShutDownButton);
  histogram_tester_->ExpectTotalCount(kUserClicksHistogramName, 1);
  histogram_tester_->ExpectBucketCount(
      kUserClicksHistogramName,
      static_cast<int>(
          LoginMetricsRecorder::LockScreenUserClickTarget::kShutDownButton),
      1);

  login_metrics_recorder_->RecordUserClickEventOnLockScreen(
      LoginMetricsRecorder::LockScreenUserClickTarget::kRestartButton);
  histogram_tester_->ExpectTotalCount(kUserClicksHistogramName, 2);
  histogram_tester_->ExpectBucketCount(
      kUserClicksHistogramName,
      static_cast<int>(
          LoginMetricsRecorder::LockScreenUserClickTarget::kRestartButton),
      1);

  login_metrics_recorder_->RecordUserClickEventOnLockScreen(
      LoginMetricsRecorder::LockScreenUserClickTarget::kSignOutButton);
  histogram_tester_->ExpectTotalCount(kUserClicksHistogramName, 3);
  histogram_tester_->ExpectBucketCount(
      kUserClicksHistogramName,
      static_cast<int>(
          LoginMetricsRecorder::LockScreenUserClickTarget::kSignOutButton),
      1);

  login_metrics_recorder_->RecordUserClickEventOnLockScreen(
      LoginMetricsRecorder::LockScreenUserClickTarget::kCloseNoteButton);
  histogram_tester_->ExpectTotalCount(kUserClicksHistogramName, 4);
  histogram_tester_->ExpectBucketCount(
      kUserClicksHistogramName,
      static_cast<int>(
          LoginMetricsRecorder::LockScreenUserClickTarget::kCloseNoteButton),
      1);

  login_metrics_recorder_->RecordUserClickEventOnLockScreen(
      LoginMetricsRecorder::LockScreenUserClickTarget::kSystemTray);
  histogram_tester_->ExpectTotalCount(kUserClicksHistogramName, 5);
  histogram_tester_->ExpectBucketCount(
      kUserClicksHistogramName,
      static_cast<int>(
          LoginMetricsRecorder::LockScreenUserClickTarget::kSystemTray),
      1);

  login_metrics_recorder_->RecordUserClickEventOnLockScreen(
      LoginMetricsRecorder::LockScreenUserClickTarget::kVirtualKeyboardTray);
  histogram_tester_->ExpectTotalCount(kUserClicksHistogramName, 6);
  histogram_tester_->ExpectBucketCount(
      kUserClicksHistogramName,
      static_cast<int>(LoginMetricsRecorder::LockScreenUserClickTarget::
                           kVirtualKeyboardTray),
      1);

  login_metrics_recorder_->RecordUserClickEventOnLockScreen(
      LoginMetricsRecorder::LockScreenUserClickTarget::kImeTray);
  histogram_tester_->ExpectTotalCount(kUserClicksHistogramName, 7);
  histogram_tester_->ExpectBucketCount(
      kUserClicksHistogramName,
      static_cast<int>(
          LoginMetricsRecorder::LockScreenUserClickTarget::kImeTray),
      1);

  login_metrics_recorder_->RecordUserClickEventOnLockScreen(
      LoginMetricsRecorder::LockScreenUserClickTarget::kNotificationTray);
  histogram_tester_->ExpectTotalCount(kUserClicksHistogramName, 8);
  histogram_tester_->ExpectBucketCount(
      kUserClicksHistogramName,
      static_cast<int>(
          LoginMetricsRecorder::LockScreenUserClickTarget::kNotificationTray),
      1);

  login_metrics_recorder_->RecordUserClickEventOnLockScreen(
      LoginMetricsRecorder::LockScreenUserClickTarget::
          kLockScreenNoteActionButton);
  histogram_tester_->ExpectTotalCount(kUserClicksHistogramName, 9);
  histogram_tester_->ExpectBucketCount(
      kUserClicksHistogramName,
      static_cast<int>(LoginMetricsRecorder::LockScreenUserClickTarget::
                           kLockScreenNoteActionButton),
      1);
}

}  // namespace ash
