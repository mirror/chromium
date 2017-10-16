// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/login_metrics_recorder.h"

#include "ash/login/ui/lock_screen.h"
#include "ash/login/ui/login_test_base.h"
#include "ash/shell.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/test/histogram_tester.h"

namespace ash {
namespace {

constexpr char kAuthMethodUsageAsTabletHistogramName[] =
    "Ash.Login.AuthMethodUsage.Lock.TabletMode";
constexpr char kAuthMethodUsageAsClamShellHistogramName[] =
    "Ash.Login.AuthMethodUsage.Lock.ClamShell";
constexpr char kNumAttemptTilSuccessHistogramName[] =
    "Ash.Login.NumPasswordAttemptUntilAuthSuccess.Lock";
constexpr char kNumAttemptTilAbandonHistogramName[] =
    "Ash.Login.NumPasswordAttemptUntilUserAbandon.Lock";
constexpr char kUserClicksHistogramName[] = "Ash.Login.UserClicks.Lock";
constexpr char kAuthMethodSwitchHistogramName[] =
    "Ash.Login.AuthMethodSwitch.Lock";

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
TEST_F(LoginMetricsRecorderTest, RecordNumAttempt) {
  login_metrics_recorder_->RecordNumAttempt(5, true /*eventual success*/);
  histogram_tester_->ExpectTotalCount(kNumAttemptTilSuccessHistogramName, 1);
  histogram_tester_->ExpectBucketCount(kNumAttemptTilSuccessHistogramName, 5,
                                       1);

  login_metrics_recorder_->RecordNumAttempt(7, false /*eventual success*/);
  histogram_tester_->ExpectTotalCount(kNumAttemptTilSuccessHistogramName, 1);
  histogram_tester_->ExpectBucketCount(kNumAttemptTilSuccessHistogramName, 5,
                                       1);
  histogram_tester_->ExpectTotalCount(kNumAttemptTilAbandonHistogramName, 1);
  histogram_tester_->ExpectBucketCount(kNumAttemptTilAbandonHistogramName, 7,
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
