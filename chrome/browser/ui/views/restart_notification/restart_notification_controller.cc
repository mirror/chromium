// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/restart_notification/restart_notification_controller.h"

#include "base/bind.h"
#include "base/logging.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace {

enum class RestartNotification {
  // Indications are via the Chrome menu only -- no work for the controller.
  kChromeMenuOnly,

  // Present the restart recommended bubble in the last active browser window.
  kRecommendedBubble,

  // Present the restart required dialog in the last active browser window.
  kRequiredDialog,
};

// Returns the policy setting, mapping out-of-range values to kChromeMenuOnly.
RestartNotification ReadPreference() {
  switch (g_browser_process->local_state()->GetInteger(
      prefs::kRestartNotification)) {
    case 1:
      return RestartNotification::kRecommendedBubble;
    case 2:
      return RestartNotification::kRequiredDialog;
  }
  return RestartNotification::kChromeMenuOnly;
}

}  // namespace

RestartNotificationController::RestartNotificationController(
    UpgradeDetector* upgrade_detector)
    : upgrade_detector_(upgrade_detector),
      last_notification_style_(NotificationStyle::kNone),
      last_level_(UpgradeDetector::UPGRADE_ANNOYANCE_NONE) {
  PrefService* local_state = g_browser_process->local_state();
  if (local_state) {
    pref_change_registrar_.Init(local_state);
    // base::Unretained is safe here because |this| outlives the registrar.
    pref_change_registrar_.Add(
        prefs::kRestartNotification,
        base::BindRepeating(&RestartNotificationController::HandleCurrentStyle,
                            base::Unretained(this)));
    // Synchronize the instance with the current state of the preference.
    HandleCurrentStyle();
  }

  // Watch for changes to the UpgradeNotificationAnnoyanceLevel.
}

RestartNotificationController::~RestartNotificationController() {
  if (last_notification_style_ != NotificationStyle::kNone)
    StopObservingUpgrades();
}

void RestartNotificationController::OnUpgradeRecommended() {
  DCHECK_NE(last_notification_style_, NotificationStyle::kNone);
  UpgradeDetector::UpgradeNotificationAnnoyanceLevel current_level =
      upgrade_detector_->upgrade_notification_stage();

  // Nothing to do if there has been no change in the level. If appropriate, a
  // notification for this level has already been shown.
  if (current_level == last_level_)
    return;

  // Handle the new level.
  switch (current_level) {
    case UpgradeDetector::UPGRADE_ANNOYANCE_NONE:
      // While it's unexpected that the level could move back down to none, it's
      // not a challenge to do the right thing.
      CloseRestartNotification();
      last_level_ = current_level;
      break;
    case UpgradeDetector::UPGRADE_ANNOYANCE_LOW:
    case UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED:
    case UpgradeDetector::UPGRADE_ANNOYANCE_HIGH:
    case UpgradeDetector::UPGRADE_ANNOYANCE_SEVERE:
      last_level_ = current_level;
      ShowRestartNotification();
      break;
    case UpgradeDetector::UPGRADE_ANNOYANCE_CRITICAL:
      // Critical notifications are handled by ToolbarView -- nothing to do.
      break;
  }
}

void RestartNotificationController::HandleCurrentStyle() {
  NotificationStyle notification_style = NotificationStyle::kNone;

  switch (ReadPreference()) {
    case RestartNotification::kChromeMenuOnly:
      DCHECK_EQ(notification_style, NotificationStyle::kNone);
      break;
    case RestartNotification::kRecommendedBubble:
      notification_style = NotificationStyle::kRecommended;
      break;
    case RestartNotification::kRequiredDialog:
      notification_style = NotificationStyle::kRequired;
      break;
  }

  // Nothing to do if there has been no change in the preference.
  if (notification_style == last_notification_style_)
    return;

  // Close the bubble or dialog if either is open.
  if (last_notification_style_ != NotificationStyle::kNone)
    CloseRestartNotification();

  // Reset state so that a notifications is shown anew in a new style if needed.
  last_level_ = UpgradeDetector::UPGRADE_ANNOYANCE_NONE;

  if (notification_style == NotificationStyle::kNone) {
    // Transition away from monitoring for upgrade events back to being dormant:
    // there is no need since AppMenuIconController takes care of updating the
    // Chrome menu as needed.
    StopObservingUpgrades();
    last_notification_style_ = notification_style;
    return;
  }

  // Transitioning away from being dormant: observe the UpgradeDetector.
  if (last_notification_style_ == NotificationStyle::kNone)
    StartObservingUpgrades();

  last_notification_style_ = notification_style;

  // Synchronize the instance with the current state of detection.
  OnUpgradeRecommended();
}

void RestartNotificationController::StartObservingUpgrades() {
  upgrade_detector_->AddObserver(this);
}

void RestartNotificationController::StopObservingUpgrades() {
  upgrade_detector_->RemoveObserver(this);
}

void RestartNotificationController::ShowRestartNotification() {
  DCHECK_NE(last_notification_style_, NotificationStyle::kNone);
  DCHECK_NE(last_level_, UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
  DCHECK_NE(last_level_, UpgradeDetector::UPGRADE_ANNOYANCE_CRITICAL);

  if (last_notification_style_ == NotificationStyle::kRecommended)
    ShowRestartRecommendedBubble();
  else
    ShowRestartRequiredDialog();
}

void RestartNotificationController::CloseRestartNotification() {
  DCHECK_NE(last_notification_style_, NotificationStyle::kNone);

  // Nothing needs to be closed if the annoyance level is none.
  if (last_level_ == UpgradeDetector::UPGRADE_ANNOYANCE_NONE)
    return;

  if (last_notification_style_ == NotificationStyle::kRecommended)
    CloseRestartRecommendedBubble();
  else
    CloseRestartRequiredDialog();
}

void RestartNotificationController::ShowRestartRecommendedBubble() {
  DCHECK_EQ(last_notification_style_, NotificationStyle::kRecommended);
  DCHECK_GT(last_level_, UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
  DCHECK_LT(last_level_, UpgradeDetector::UPGRADE_ANNOYANCE_CRITICAL);
  // TODO(grt): implement.
}

void RestartNotificationController::CloseRestartRecommendedBubble() {
  DCHECK_EQ(last_notification_style_, NotificationStyle::kRecommended);
  DCHECK_GT(last_level_, UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
  DCHECK_LT(last_level_, UpgradeDetector::UPGRADE_ANNOYANCE_CRITICAL);
  // TODO(grt): implement.
}

void RestartNotificationController::ShowRestartRequiredDialog() {
  DCHECK_EQ(last_notification_style_, NotificationStyle::kRequired);
  DCHECK_GT(last_level_, UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
  DCHECK_LT(last_level_, UpgradeDetector::UPGRADE_ANNOYANCE_CRITICAL);
  // TODO(grt): implement.
}

void RestartNotificationController::CloseRestartRequiredDialog() {
  DCHECK_EQ(last_notification_style_, NotificationStyle::kRequired);
  DCHECK_GT(last_level_, UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
  DCHECK_LT(last_level_, UpgradeDetector::UPGRADE_ANNOYANCE_CRITICAL);
  // TODO(grt): implement.
}
