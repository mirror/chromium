// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/off_hours/tray_off_hours_session_limit.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/system_notifier.h"
#include "ash/system/tray/label_tray_view.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "ash/system/tray/tray_constants.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/time_format.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/notification.h"
#include "ui/views/view.h"

namespace ash {
namespace {

// If the remaining session time falls below this threshold, the user should be
// informed that the session is about to expire.
const int kExpiringSoonThresholdInMinutes = 5;

// Use 500ms interval for updates to notification and tray bubble to reduce the
// likelihood of a user-visible skip in high load situations (as might happen
// with 1000ms).
const int kTimerIntervalInMilliseconds = 500;

}  // namespace

// static
const char TrayOffHoursSessionLimit::kNotificationId[] =
    "chrome://session/timeout";

TrayOffHoursSessionLimit::TrayOffHoursSessionLimit(SystemTray* system_tray)
    : SystemTrayItem(system_tray, UMA_OFF_HOURS_LIMIT),
      off_hours_mode_(OFF_HOURS_OFF),
      last_off_hours_mode_(OFF_HOURS_OFF),
      tray_bubble_view_(nullptr) {
  Shell::Get()->session_controller()->AddObserver(this);
  Update();
}

TrayOffHoursSessionLimit::~TrayOffHoursSessionLimit() {
  Shell::Get()->session_controller()->RemoveObserver(this);
}

// Add view to tray bubble.
views::View* TrayOffHoursSessionLimit::CreateDefaultView(LoginStatus status) {
  LOG(ERROR) << "Daria: CreateDefaultView";
  CHECK(!tray_bubble_view_);
  UpdateState();
  if (off_hours_mode_ == OFF_HOURS_OFF)
    return nullptr;
  tray_bubble_view_ = new LabelTrayView(nullptr, kSystemMenuTimerIcon);
  tray_bubble_view_->SetMessage(ComposeTrayBubbleMessage());
  return tray_bubble_view_;
}

// View has been removed from tray bubble.
void TrayOffHoursSessionLimit::OnDefaultViewDestroyed() {
  tray_bubble_view_ = nullptr;
}

void TrayOffHoursSessionLimit::OnSessionStateChanged(
    session_manager::SessionState state) {
  Update();
}

void TrayOffHoursSessionLimit::OnOffHoursModeChanged() {
  LOG(ERROR) << "Daria: Off Hours Mode Changed";
  Update();
}

void TrayOffHoursSessionLimit::Update() {
  // Don't show notification or tray item until the user is logged in.
  //  LOG(ERROR) << "Daria: off hours mode = " <<
  //  Shell::Get()->session_controller()->off_hours_mode();
  if (!Shell::Get()->session_controller()->off_hours_mode() &&
      off_hours_mode_ == OFF_HOURS_OFF) {
    return;
  }
  UpdateState();
  UpdateNotification();
  UpdateTrayBubbleView();
}

void TrayOffHoursSessionLimit::UpdateState() {
  SessionController* session = Shell::Get()->session_controller();
  base::TimeDelta time_limit = session->off_hours_limit();
  base::TimeTicks session_start_time = session->off_hours_start_time();
  bool off_hours_mode = session->off_hours_mode();

  if (off_hours_mode && !time_limit.is_zero()) {
    const base::TimeDelta expiring_soon_threshold(
        base::TimeDelta::FromMinutes(kExpiringSoonThresholdInMinutes));
    remaining_session_time_ =
        std::max(time_limit - (base::TimeTicks::Now() - session_start_time),
                 base::TimeDelta());

    //    LOG(ERROR) << "Daria: remaining time = " << remaining_session_time_;
    off_hours_mode_ = remaining_session_time_ <= expiring_soon_threshold
                          ? OFF_HOURS_EXPIRING_SOON
                          : OFF_HOURS_ON;
    if (!timer_)
      timer_.reset(new base::RepeatingTimer);
    if (!timer_->IsRunning()) {
      timer_->Start(
          FROM_HERE,
          base::TimeDelta::FromMilliseconds(kTimerIntervalInMilliseconds), this,
          &TrayOffHoursSessionLimit::Update);
    }
  } else {
    remaining_session_time_ = base::TimeDelta();
    off_hours_mode_ = OFF_HOURS_OFF;
    timer_.reset();
  }
}

void TrayOffHoursSessionLimit::UpdateNotification() {
  message_center::MessageCenter* message_center =
      message_center::MessageCenter::Get();

  // If state hasn't changed and the notification has already been acknowledged,
  // we won't re-create it.
  if (off_hours_mode_ == last_off_hours_mode_ &&
      !message_center->FindVisibleNotificationById(kNotificationId)) {
    return;
  }

  // After state change, any possibly existing notification is removed to make
  // sure it is re-shown even if it had been acknowledged by the user before
  // (and in the rare case of state change towards OFF_HOURS_OFF to make the
  // notification disappear).
  if (off_hours_mode_ != last_off_hours_mode_ &&
      message_center->FindVisibleNotificationById(kNotificationId)) {
    message_center::MessageCenter::Get()->RemoveNotification(
        kNotificationId, false /* by_user */);
  }

  // For LIMIT_NONE, there's nothing more to do.
  if (off_hours_mode_ == OFF_HOURS_OFF) {
    last_off_hours_mode_ = off_hours_mode_;
    return;
  }

  message_center::RichNotificationData data;
  data.should_make_spoken_feedback_for_popup_updates =
      (off_hours_mode_ != last_off_hours_mode_);
  std::unique_ptr<message_center::Notification> notification =
      system_notifier::CreateSystemNotification(
          message_center::NOTIFICATION_TYPE_SIMPLE, kNotificationId,
          base::string16() /* title */,
          ComposeNotificationMessage() /* message */,
          gfx::Image(
              gfx::CreateVectorIcon(kSystemMenuTimerIcon, kMenuIconColor)),
          base::string16() /* display_source */, GURL(),
          message_center::NotifierId(
              message_center::NotifierId::SYSTEM_COMPONENT,
              system_notifier::kNotifierSessionLengthTimeout),
          data, nullptr /* delegate */, kNotificationTimerIcon,
          message_center::SystemNotificationWarningLevel::NORMAL);
  notification->SetSystemPriority();
  if (message_center->FindVisibleNotificationById(kNotificationId)) {
    message_center->UpdateNotification(kNotificationId,
                                       std::move(notification));
  } else {
    message_center->AddNotification(std::move(notification));
  }
  last_off_hours_mode_ = off_hours_mode_;
}

void TrayOffHoursSessionLimit::UpdateTrayBubbleView() const {
  if (!tray_bubble_view_)
    return;
  if (off_hours_mode_ == OFF_HOURS_OFF)
    tray_bubble_view_->SetMessage(base::string16());
  else
    tray_bubble_view_->SetMessage(ComposeTrayBubbleMessage());
  tray_bubble_view_->Layout();
}

base::string16 TrayOffHoursSessionLimit::ComposeNotificationMessage() const {
  return l10n_util::GetStringFUTF16(
      IDS_ASH_STATUS_TRAY_NOTIFICATION_SESSION_LENGTH_LIMIT,
      ui::TimeFormat::Detailed(ui::TimeFormat::FORMAT_DURATION,
                               ui::TimeFormat::LENGTH_LONG, 10,
                               remaining_session_time_));
}

base::string16 TrayOffHoursSessionLimit::ComposeTrayBubbleMessage() const {
  return l10n_util::GetStringFUTF16(
      IDS_ASH_STATUS_TRAY_BUBBLE_SESSION_LENGTH_LIMIT,
      ui::TimeFormat::Detailed(ui::TimeFormat::FORMAT_DURATION,
                               ui::TimeFormat::LENGTH_LONG, 10,
                               remaining_session_time_));
}

}  // namespace ash
