// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/web_notification/login_state_notification_blocker.h"

#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/system/system_notifier.h"
#include "ui/message_center/message_center.h"

using session_manager::SessionState;

namespace ash {

namespace {

// Returns true if session state is active, there is an active user and the
// PrefService of the active user is initialized.
bool ShouldShowNotificationForActiveUserSession() {
  SessionController* const session_controller =
      Shell::Get()->session_controller();
  if (session_controller->GetSessionState() != SessionState::ACTIVE)
    return false;

  const auto* active_user_session = session_controller->GetUserSession(0);
  if (!active_user_session)
    return false;

  return session_controller->GetUserPrefServiceForUser(
      active_user_session->user_info->account_id);
}

}  // namespace

LoginStateNotificationBlocker::LoginStateNotificationBlocker(
    message_center::MessageCenter* message_center)
    : NotificationBlocker(message_center) {
  Shell::Get()->session_controller()->AddObserver(this);
}

LoginStateNotificationBlocker::~LoginStateNotificationBlocker() {
  Shell::Get()->session_controller()->RemoveObserver(this);
}

bool LoginStateNotificationBlocker::ShouldShowNotification(
    const message_center::Notification& notification) const {
  return should_show_notification_;
}

bool LoginStateNotificationBlocker::ShouldShowNotificationAsPopup(
    const message_center::Notification& notification) const {
  if (ash::system_notifier::ShouldAlwaysShowPopups(notification.notifier_id()))
    return true;

  return should_show_notification_;
}

void LoginStateNotificationBlocker::OnSessionStateChanged(SessionState state) {
  CheckStateAndNotifyIfChanged();
}

void LoginStateNotificationBlocker::OnActiveUserPrefServiceChanged(
    PrefService* pref_service) {
  CheckStateAndNotifyIfChanged();
}

void LoginStateNotificationBlocker::CheckStateAndNotifyIfChanged() {
  const bool new_should_show_notification =
      ShouldShowNotificationForActiveUserSession();
  if (new_should_show_notification == should_show_notification_)
    return;

  should_show_notification_ = new_should_show_notification;
  NotifyBlockingStateChanged();
}

}  // namespace ash
