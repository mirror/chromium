// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/metrics/login_metrics_recorder.h"

#include "ash/login/ui/lock_screen.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"

namespace ash {

namespace {

void LogUserClickOnLock(
    LoginMetricsRecorder::LockScreenUserClickTarget target) {
  UMA_HISTOGRAM_ENUMERATION(
      "Ash.Login.Lock.UserClicks", target,
      LoginMetricsRecorder::LockScreenUserClickTarget::kTargetCount);
}

void LogUserClickOnLogin(
    LoginMetricsRecorder::LoginScreenUserClickTarget target) {
  UMA_HISTOGRAM_ENUMERATION(
      "Ash.Login.Login.UserClicks", target,
      LoginMetricsRecorder::LoginScreenUserClickTarget::kTargetCount);
}

bool ShouldRecordMetrics(session_manager::SessionState session_state) {
  return session_state == session_manager::SessionState::LOGIN_PRIMARY ||
         session_state == session_manager::SessionState::LOCKED;
}

}  // namespace

LoginMetricsRecorder::LoginMetricsRecorder() = default;
LoginMetricsRecorder::~LoginMetricsRecorder() = default;

void LoginMetricsRecorder::SetAuthMethod(AuthMethod method) {
  DCHECK_NE(method, AuthMethod::kMethodCount);
  if (!ShouldRecordMetrics(
          Shell::Get()->session_controller()->GetSessionState()))
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
  if (!ShouldRecordMetrics(
          Shell::Get()->session_controller()->GetSessionState()))
    return;

  if (success) {
    UMA_HISTOGRAM_COUNTS_100("Ash.Login.Lock.NumPasswordAttempts.UntilSuccess",
                             num_attempt);
  } else {
    UMA_HISTOGRAM_COUNTS_100("Ash.Login.Lock.NumPasswordAttempts.UntilFailure",
                             num_attempt);
  }
}

void LoginMetricsRecorder::RecordUserTrayClick(TrayClickTarget target) {
  session_manager::SessionState session_state =
      Shell::Get()->session_controller()->GetSessionState();
  if (!ShouldRecordMetrics(session_state))
    return;

  bool is_lock = session_state == session_manager::SessionState::LOCKED;
  switch (target) {
    case TrayClickTarget::kSystemTray:
      if (is_lock) {
        LogUserClickOnLock(LockScreenUserClickTarget::kSystemTray);
      } else {
        LogUserClickOnLogin(LoginScreenUserClickTarget::kSystemTray);
      }
      break;
    case TrayClickTarget::kVirtualKeyboardTray:
      if (is_lock) {
        LogUserClickOnLock(LockScreenUserClickTarget::kVirtualKeyboardTray);
      } else {
        LogUserClickOnLogin(LoginScreenUserClickTarget::kVirtualKeyboardTray);
      }
      break;
    case TrayClickTarget::kImeTray:
      if (is_lock) {
        LogUserClickOnLock(LockScreenUserClickTarget::kImeTray);
      } else {
        LogUserClickOnLogin(LoginScreenUserClickTarget::kImeTray);
      }
      break;
    case TrayClickTarget::kNotificationTray:
      DCHECK(is_lock);
      LogUserClickOnLock(LockScreenUserClickTarget::kNotificationTray);
      break;
    case TrayClickTarget::kTrayActionNoteButton:
      DCHECK(is_lock);
      LogUserClickOnLock(
          LockScreenUserClickTarget::kLockScreenNoteActionButton);
      break;
  }
}

void LoginMetricsRecorder::RecordUserShelfButtonClick(
    ShelfButtonClickTarget target) {
  session_manager::SessionState session_state =
      Shell::Get()->session_controller()->GetSessionState();
  if (!ShouldRecordMetrics(session_state))
    return;

  bool is_lock = session_state == session_manager::SessionState::LOCKED;
  switch (target) {
    case ShelfButtonClickTarget::kShutDownButton:
      if (is_lock) {
        LogUserClickOnLock(LockScreenUserClickTarget::kShutDownButton);
      } else {
        LogUserClickOnLogin(LoginScreenUserClickTarget::kShutDownButton);
      }
      break;
    case ShelfButtonClickTarget::kRestartButton:
      if (is_lock) {
        LogUserClickOnLock(LockScreenUserClickTarget::kRestartButton);
      } else {
        LogUserClickOnLogin(LoginScreenUserClickTarget::kRestartButton);
      }
      break;
    case ShelfButtonClickTarget::kSignOutButton:
      DCHECK(is_lock);
      LogUserClickOnLock(LockScreenUserClickTarget::kSignOutButton);
      break;
    case ShelfButtonClickTarget::kBrowseAsGuestButton:
      DCHECK(!is_lock);
      LogUserClickOnLogin(LoginScreenUserClickTarget::kBrowseAsGuestButton);
      break;
    case ShelfButtonClickTarget::kAddUserButton:
      DCHECK(!is_lock);
      LogUserClickOnLogin(LoginScreenUserClickTarget::kAddUserButton);
      break;
    case ShelfButtonClickTarget::kCloseNoteButton:
      DCHECK(is_lock);
      LogUserClickOnLock(LockScreenUserClickTarget::kCloseNoteButton);
      break;
    case ShelfButtonClickTarget::kCancelButton:
      // Should not be called in LOCKED nor LOGIN_PRIMARY states.
      NOTREACHED();
      break;
  }
}

// static
LoginMetricsRecorder::AuthMethodSwitchType LoginMetricsRecorder::FindSwitchType(
    AuthMethod previous,
    AuthMethod current) {
  DCHECK_NE(previous, current);
  switch (previous) {
    case AuthMethod::kPassword:
      return current == AuthMethod::kPin
                 ? AuthMethodSwitchType::kPasswordToPin
                 : AuthMethodSwitchType::kPasswordToSmartlock;
    case AuthMethod::kPin:
      return current == AuthMethod::kPassword
                 ? AuthMethodSwitchType::kPinToPassword
                 : AuthMethodSwitchType::kPinToSmartlock;
    case AuthMethod::kSmartlock:
      return current == AuthMethod::kPassword
                 ? AuthMethodSwitchType::kSmartlockToPassword
                 : AuthMethodSwitchType::kSmartlockToPin;
    case AuthMethod::kMethodCount:
      NOTREACHED();
      return AuthMethodSwitchType::kSwitchTypeCount;
  }
}

}  // namespace ash
