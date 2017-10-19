// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/metrics/login_metrics_recorder.h"

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
  DCHECK_NE(previous, current);
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
    case LoginMetricsRecorder::AuthMethod::kMethodCount:
      NOTREACHED();
      return LoginMetricsRecorder::AuthMethodSwitchType::kSwitchTypeCount;
  }
}

}  // namespace

LoginMetricsRecorder::LoginMetricsRecorder() = default;
LoginMetricsRecorder::~LoginMetricsRecorder() = default;

void LoginMetricsRecorder::SetAuthMethod(AuthMethod method) {
  DCHECK_NE(method, AuthMethod::kMethodCount);
  if (!ash::LockScreen::IsShown())
    return;

  // Record usage of PIN / Password / Smartlock in lock screen.
  const bool is_tablet_mode = Shell::Get()
                                  ->tablet_mode_controller()
                                  ->IsTabletModeWindowManagerEnabled();
  if (is_tablet_mode) {
    UMA_HISTOGRAM_ENUMERATION("Ash.Login.Lock.AuthMethod.Used.TabletMode",
                              method, AuthMethod::kMethodCount);
  } else {
    UMA_HISTOGRAM_ENUMERATION("Ash.Login.Lock.AuthMethod.Used.ClamShellMode",
                              method, AuthMethod::kMethodCount);
  }

  if (last_auth_method_ != method) {
    // Record switching between unlock methods.
    UMA_HISTOGRAM_ENUMERATION("Ash.Login.Lock.AuthMethod.Switched",
                              FindSwitchType(last_auth_method_, method),
                              AuthMethodSwitchType::kSwitchTypeCount);

    last_auth_method_ = method;
  }
}

void LoginMetricsRecorder::Reset() {
  // Reset local state.
  last_auth_method_ = AuthMethod::kPassword;
}

void LoginMetricsRecorder::RecordNumLoginAttempts(int num_attempt,
                                                  bool success) {
  if (!ash::LockScreen::IsShown())
    return;

  if (success) {
    UMA_HISTOGRAM_COUNTS_100("Ash.Login.Lock.NumPasswordAttempts.UntilSuccess",
                             num_attempt);
  } else {
    UMA_HISTOGRAM_COUNTS_100("Ash.Login.Lock.NumPasswordAttempts.UntilFailure",
                             num_attempt);
  }
}

void LoginMetricsRecorder::RecordUserClickEventOnLockScreen(
    LockScreenUserClickTarget target) {
  DCHECK_NE(target, LockScreenUserClickTarget::kTargetCount);
  DCHECK(ash::LockScreen::IsShown());

  UMA_HISTOGRAM_ENUMERATION("Ash.Login.Lock.UserClicks", target,
                            LockScreenUserClickTarget::kTargetCount);
}

}  // namespace ash
