// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/critical_notification_bubble_view.h"

#include <math.h>

#include "base/logging.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/prefs/force_restart_after_update_pref.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/upgrade_detector.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/locale_settings.h"
#include "components/prefs/pref_service.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"

using base::UserMetricsAction;

namespace {

enum class Style {
  kCriticalUpdate,
  kRestartOnIdleByPolicy,
  kRecurringRestartByPolicy,
  kForceRestartByPolicy,
  NUM_STYLES
};

struct StyleProperties {
  Style style;
  chrome::DialogIdentifier dialog_identifier;
  bool is_policy_controlled;
  bool can_cancel;
  base::TimeDelta countdown_delta;
  int minutes_left_msg;
  int seconds_left_msg;
  int restart_now_msg;
  int body_msg;
  int ok_button_id;
  int cancel_button_id;
  base::UserMetricsAction show_action;
  base::UserMetricsAction ok_action;
  base::UserMetricsAction cancel_action;
  base::UserMetricsAction auto_restart_action;
};

// Keep this in sync with CriticalNotificationBubbleView::Style.
constexpr StyleProperties kProperties[] = {
    {
        Style::kCriticalUpdate, chrome::DialogIdentifier::CRITICAL_NOTIFICATION,
        false /* !is_policy_controlled */, true /* can_cancel */,
        base::TimeDelta::FromSeconds(30), 0 /* minutes_left_msg */,
        IDS_CRITICAL_NOTIFICATION_HEADLINE,
        IDS_CRITICAL_NOTIFICATION_HEADLINE_ALTERNATE,
        IDS_CRITICAL_NOTIFICATION_TEXT, IDS_CRITICAL_NOTIFICATION_RESTART_NOW,
        IDS_CRITICAL_NOTIFICATION_DISMISS,
        base::UserMetricsAction("CriticalNotificationShown"),
        base::UserMetricsAction("CriticalNotification_Restart"),
        base::UserMetricsAction("CriticalNotification_Ignore"),
        base::UserMetricsAction("CriticalNotification_AutoRestart"),
    },
    {
        Style::kRestartOnIdleByPolicy,
        chrome::DialogIdentifier::RESTART_ON_IDLE_BY_POLICY,
        true /* is_policy_controlled */, true /* can_cancel */,
        base::TimeDelta::FromSeconds(30),
        IDS_RESTART_BY_POLICY_HEADLINE_MINUTES,
        IDS_RESTART_BY_POLICY_HEADLINE_SECONDS,
        IDS_RESTART_BY_POLICY_HEADLINE_ALTERNATE,
        IDS_RESTART_BY_POLICY_NOTIFICATION_TEXT,
        IDS_CRITICAL_NOTIFICATION_RESTART, IDS_CRITICAL_NOTIFICATION_DISMISS,
        base::UserMetricsAction("RestartOnIdleNotificationShown"),
        base::UserMetricsAction("RestartOnIdleNotification_Restart"),
        base::UserMetricsAction("RestartOnIdleNotification_Ignore"),
        base::UserMetricsAction("RestartOnIdleNotification_AutoRestart"),
    },
    {
        Style::kRecurringRestartByPolicy,
        chrome::DialogIdentifier::RECURRING_RESTART_BY_POLICY,
        true /* is_policy_controlled */, true /* can_cancel */,
        base::TimeDelta::FromMinutes(3), IDS_RESTART_BY_POLICY_HEADLINE_MINUTES,
        IDS_RESTART_BY_POLICY_HEADLINE_SECONDS,
        IDS_RESTART_BY_POLICY_HEADLINE_ALTERNATE,
        IDS_RESTART_BY_POLICY_NOTIFICATION_TEXT,
        IDS_CRITICAL_NOTIFICATION_RESTART, IDS_RESTART_IN_ONE_HOUR,
        base::UserMetricsAction("RecurringRestartNotificationShown"),
        base::UserMetricsAction("RecurringRestartNotification_Restart"),
        base::UserMetricsAction("RecurringRestartNotification_Ignore"),
        base::UserMetricsAction("RecurringRestartNotification_AutoRestart"),
    },
    {
        Style::kForceRestartByPolicy,
        chrome::DialogIdentifier::FORCE_RESTART_BY_POLICY,
        true /* is_policy_controlled */, false /* !can_cancel */,
        base::TimeDelta::FromMinutes(3), IDS_RESTART_BY_POLICY_HEADLINE_MINUTES,
        IDS_RESTART_BY_POLICY_HEADLINE_SECONDS,
        IDS_RESTART_BY_POLICY_HEADLINE_ALTERNATE,
        IDS_FORCE_RESTART_BY_POLICY_NOTIFICATION_TEXT, IDS_FORCE_RESTART_NOW, 0,
        base::UserMetricsAction("ForceRestartNotificationShown"),
        base::UserMetricsAction("ForceRestartNotification_Restart"),
        base::UserMetricsAction(nullptr),
        base::UserMetricsAction("ForceRestartNotification_AutoRestart"),
    }};

// Returns an index into kProperties based on the ForceRestartAfterUpdate policy
// setting.
int DetermineStyleIndex() {
  static_assert(sizeof(kProperties) / sizeof(StyleProperties) ==
                    static_cast<int>(Style::NUM_STYLES),
                "kProperties out of sync with enum Style");

  PrefService* local_state = g_browser_process->local_state();
  if (local_state) {
    switch (GetForceRestartAfterUpdatePref(local_state)) {
      case ForceRestartAfterUpdatePref::kUnset:
        break;
      case ForceRestartAfterUpdatePref::kRestartWhenIdle:
        static_assert(kProperties[1].style == Style::kRestartOnIdleByPolicy,
                      "kProperties mismatch");
        return 1;
      case ForceRestartAfterUpdatePref::kDeferrableRestart:
        static_assert(kProperties[2].style == Style::kRecurringRestartByPolicy,
                      "kProperties mismatch");
        return 2;
      case ForceRestartAfterUpdatePref::kForcedRestart:
        static_assert(kProperties[3].style == Style::kForceRestartByPolicy,
                      "kProperties mismatch");
        return 3;
    }
  }
  static_assert(kProperties[0].style == Style::kCriticalUpdate,
                "kProperties mismatch");
  return 0;
}

// Returns the bubble properties constants for |style|.
const StyleProperties& GetProperties(int style_index) {
  DCHECK_GE(style_index, 0);
  DCHECK_LT(static_cast<size_t>(style_index), arraysize(kProperties));
  return kProperties[style_index];
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// CriticalNotificationBubbleView

CriticalNotificationBubbleView::CriticalNotificationBubbleView(
    views::View* anchor_view)
    : BubbleDialogDelegateView(anchor_view, views::BubbleBorder::TOP_RIGHT),
      style_index_(DetermineStyleIndex()) {
  set_close_on_deactivate(false);
  chrome::RecordDialogCreation(GetProperties(style_index_).dialog_identifier);
}

CriticalNotificationBubbleView::~CriticalNotificationBubbleView() {
}

base::TimeDelta CriticalNotificationBubbleView::GetRemainingTime() const {
  base::TimeDelta time_lapsed = base::TimeTicks::Now() - bubble_created_;
  return GetProperties(style_index_).countdown_delta - time_lapsed;
}

void CriticalNotificationBubbleView::ScheduleNextCountdownRefresh(
    base::TimeDelta remaining_time) {
  // Refresh at the next minute or second boundary, depending on the time left.
  const base::TimeDelta delta =
      remaining_time % (remaining_time.InSecondsF() > 60.0
                            ? base::TimeDelta::FromMinutes(1)
                            : base::TimeDelta::FromSeconds(1));

  refresh_timer_.Start(FROM_HERE, delta, this,
                       &CriticalNotificationBubbleView::OnCountdown);
}

void CriticalNotificationBubbleView::OnCountdown() {
  UpgradeDetector* upgrade_detector = UpgradeDetector::GetInstance();
  if (upgrade_detector->critical_update_acknowledged()) {
    // The user has already interacted with the bubble and chosen a path.
    GetWidget()->Close();
    return;
  }

  const base::TimeDelta remaining_time = GetRemainingTime();
  int seconds = round(remaining_time.InSecondsF());
  if (seconds <= 0) {
    // Time's up!
    upgrade_detector->acknowledge_critical_update();

    base::RecordAction(GetProperties(style_index_).auto_restart_action);
    chrome::AttemptRestart();
  } else {
    ScheduleNextCountdownRefresh(remaining_time);
  }

  // Update the counter. It may seem counter-intuitive to update the message
  // after we attempt restart, but remember that shutdown may be aborted by
  // an onbeforeunload handler, leaving the bubble up when the browser should
  // have restarted (giving the user another chance).
  GetBubbleFrameView()->UpdateWindowTitle();
}

base::string16 CriticalNotificationBubbleView::GetWindowTitle() const {
  const auto& properties = GetProperties(style_index_);
  const int seconds = round(GetRemainingTime().InSecondsF());
  if (seconds > 60 && properties.minutes_left_msg) {
    return l10n_util::GetPluralStringFUTF16(properties.minutes_left_msg,
                                            (seconds + 59) / 60);
  }
  if (seconds > 0) {
    return l10n_util::GetPluralStringFUTF16(properties.seconds_left_msg,
                                            seconds);
  }
  return l10n_util::GetStringUTF16(properties.restart_now_msg);
}

gfx::ImageSkia CriticalNotificationBubbleView::GetWindowIcon() {
  // 24 scales the vector icon by half and is quite close to 22, the height of
  // the heading text.
  return gfx::CreateVectorIcon(gfx::IconDescription(
      vector_icons::kBusinessIcon, 24, gfx::kChromeIconGrey, base::TimeDelta(),
      gfx::kNoneIcon));
}

bool CriticalNotificationBubbleView::ShouldShowWindowIcon() const {
  return true;
}

void CriticalNotificationBubbleView::WindowClosing() {
  refresh_timer_.Stop();
}

bool CriticalNotificationBubbleView::Cancel() {
  UpgradeDetector::GetInstance()->acknowledge_critical_update();
  base::RecordAction(GetProperties(style_index_).cancel_action);
  // If the counter reaches 0, we set a restart flag that must be cleared if
  // the user selects, for example, "Stay on this page" during an
  // onbeforeunload handler.
  PrefService* prefs = g_browser_process->local_state();
  if (prefs->HasPrefPath(prefs::kRestartLastSessionOnShutdown))
    prefs->ClearPref(prefs::kRestartLastSessionOnShutdown);
  return true;
}

bool CriticalNotificationBubbleView::Accept() {
  UpgradeDetector::GetInstance()->acknowledge_critical_update();
  base::RecordAction(GetProperties(style_index_).ok_action);
  chrome::AttemptRestart();
  return true;
}

base::string16 CriticalNotificationBubbleView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  const auto& properties = GetProperties(style_index_);
  return l10n_util::GetStringUTF16(button == ui::DIALOG_BUTTON_CANCEL
                                       ? properties.cancel_button_id
                                       : properties.ok_button_id);
}

void CriticalNotificationBubbleView::Init() {
  const auto& properties = GetProperties(style_index_);
  bubble_created_ = base::TimeTicks::Now();

  SetLayoutManager(std::make_unique<views::FillLayout>());

  views::Label* message = new views::Label();
  message->SetMultiLine(true);
  message->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  message->SetText(l10n_util::GetStringUTF16(properties.body_msg));
  message->SizeToFit(views::Widget::GetLocalizedContentsWidth(
      IDS_CRUCIAL_NOTIFICATION_BUBBLE_WIDTH_CHARS));
  AddChildView(message);

  ScheduleNextCountdownRefresh(properties.countdown_delta);

  base::RecordAction(properties.show_action);
}

void CriticalNotificationBubbleView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  node_data->role = ui::AX_ROLE_ALERT;
}

void CriticalNotificationBubbleView::ViewHierarchyChanged(
    const ViewHierarchyChangedDetails& details) {
  if (details.is_add && details.child == this)
    NotifyAccessibilityEvent(ui::AX_EVENT_ALERT, true);
}

int CriticalNotificationBubbleView::GetDialogButtons() const {
  return GetProperties(style_index_).can_cancel
             ? (ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL)
             : ui::DIALOG_BUTTON_OK;
}
