// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/login_metrics_recorder.h"

#include "ash/login/ui/lock_screen.h"
#include "ash/shell.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"

namespace ash {

namespace {

LoginMetricsRecorder::AuthMethodSwitchType FindSwitchType(
    LoginMetricsRecorder::AuthMethod previous,
    LoginMetricsRecorder::AuthMethod current) {
  switch (previous) {
    case LoginMetricsRecorder::AuthMethod::kPassword:
      return current == LoginMetricsRecorder::AuthMethod::kPin
                 ? LoginMetricsRecorder::AuthMethodSwitchType::kPasswordToPin
                 : LoginMetricsRecorder::AuthMethodSwitchType::
                       kPasswordToSmartlock;
    case LoginMetricsRecorder::AuthMethod::kPin:
      return current == LoginMetricsRecorder::AuthMethod::kPassword
                 ? LoginMetricsRecorder::AuthMethodSwitchType::kPinToPassword
                 : LoginMetricsRecorder::AuthMethodSwitchType::kPinToSmartlock;
    case LoginMetricsRecorder::AuthMethod::kSmartlock:
      return current == LoginMetricsRecorder::AuthMethod::kPassword
                 ? LoginMetricsRecorder::AuthMethodSwitchType::
                       kSmartlockToPassword
                 : LoginMetricsRecorder::AuthMethodSwitchType::kSmartlockToPin;
    default:
      return LoginMetricsRecorder::AuthMethodSwitchType::kSwitchTypeCount;
  }
}

}  // namespace

LoginMetricsRecorder::LoginMetricsRecorder() = default;
LoginMetricsRecorder::~LoginMetricsRecorder() = default;

void LoginMetricsRecorder::SetAuthMethod(AuthMethod method) {
  if (!ash::LockScreen::IsShown() || method == AuthMethod::kMethodCount)
    return;

  // Record usage of PIN / Password / Smartlock in lock screen.
  const bool is_tablet_mode = Shell::Get()
                                  ->tablet_mode_controller()
                                  ->IsTabletModeWindowManagerEnabled();
  if (is_tablet_mode) {
    UMA_HISTOGRAM_ENUMERATION("Ash.Login.AuthMethodUsage.Lock.TabletMode",
                              method, AuthMethod::kMethodCount);
  } else {
    UMA_HISTOGRAM_ENUMERATION("Ash.Login.AuthMethodUsage.Lock.ClamShell",
                              method, AuthMethod::kMethodCount);
  }

  if (last_auth_method_ != method) {
    // Record switching between unlock methods.
    UMA_HISTOGRAM_ENUMERATION("Ash.Login.AuthMethodSwitch.Lock",
                              FindSwitchType(last_auth_method_, method),
                              AuthMethodSwitchType::kSwitchTypeCount);

    last_auth_method_ = method;
  }
}

void LoginMetricsRecorder::OnLockStateChanged() {
  // Reset local state.
  last_auth_method_ = AuthMethod::kPassword;
}

void LoginMetricsRecorder::RecordNumAttempt(int num_attempt,
                                            bool eventual_success) {
  if (!ash::LockScreen::IsShown())
    return;

  if (num_attempt == 0 && !eventual_success)
    return;

  if (eventual_success) {
    UMA_HISTOGRAM_COUNTS_100(
        "Ash.Login.NumPasswordAttemptUntilAuthSuccess.Lock", num_attempt);
  } else {
    UMA_HISTOGRAM_COUNTS_100(
        "Ash.Login.NumPasswordAttemptUntilUserAbandon.Lock", num_attempt);
  }

  last_auth_method_ = AuthMethod::kPassword;
}

void LoginMetricsRecorder::RecordUserClickEventOnLockScreen(
    LockScreenUserClickTarget target) {
  if (!ash::LockScreen::IsShown() ||
      target == LockScreenUserClickTarget::kTargetCount) {
    return;
  }

  UMA_HISTOGRAM_ENUMERATION("Ash.Login.UserClicks.Lock", target,
                            LockScreenUserClickTarget::kTargetCount);
}

}  // namespace ash
