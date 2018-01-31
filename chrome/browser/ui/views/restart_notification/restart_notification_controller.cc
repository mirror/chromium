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
      last_level_(UpgradeDetector::UPGRADE_ANNOYANCE_NONE),
      observing_upgrades_(false) {
  PrefService* local_state = g_browser_process->local_state();
  if (local_state) {
    pref_change_registrar_.Init(local_state);
    // base::Unretained is safe here because |this| outlives the registrar.
    pref_change_registrar_.Add(
        prefs::kRestartNotification,
        base::BindRepeating(&RestartNotificationController::OnPreferenceChanged,
                            base::Unretained(this)));
    // Synchronize the instance with the current state of the preference.
    OnPreferenceChanged();
  }

  // Watch for changes to the UpgradeNotificationAnnoyanceLevel.
}

RestartNotificationController::~RestartNotificationController() {
  StopObservingUpgrades();
}

void RestartNotificationController::OnUpgradeRecommended() {
  DCHECK_NE(last_notification_style_, NotificationStyle::kNone);
  UpgradeDetector::UpgradeNotificationAnnoyanceLevel current_level =
      upgrade_detector_->upgrade_notification_stage();

  // Nothing to do if there has been no change in the level.
  if (current_level == last_level_)
    return;

  // Handle the new level.
  last_level_ = current_level;
  switch (current_level) {
    case UpgradeDetector::UPGRADE_ANNOYANCE_NONE:
      // While it's unexpected that the level could move back down to none, it's
      // not a challenge to do the right thing.
      CloseRestartNotification();
      break;
    case UpgradeDetector::UPGRADE_ANNOYANCE_LOW:
    case UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED:
    case UpgradeDetector::UPGRADE_ANNOYANCE_HIGH:
    case UpgradeDetector::UPGRADE_ANNOYANCE_SEVERE:
      ShowRestartNotification();
      break;
    case UpgradeDetector::UPGRADE_ANNOYANCE_CRITICAL:
      // Critical notifications are handled by ToolbarView -- nothing to do.
      break;
  }
}

void RestartNotificationController::ShowRestartRecommendedBubble() {
  DCHECK_EQ(last_notification_style_, NotificationStyle::kRecommended);
}

void RestartNotificationController::ShowRestartRequiredDialog() {
  DCHECK_EQ(last_notification_style_, NotificationStyle::kRequired);
}

void RestartNotificationController::OnPreferenceChanged() {
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
  CloseRestartNotification();

  last_notification_style_ = notification_style;
  if (notification_style == NotificationStyle::kNone) {
    // There is no need to watch for events from the detector;
    // AppMenuIconController takes care of updating the Chrome menu as needed.
    StopObservingUpgrades();
  } else {
    // Observe the UpgradeDetector from now on.
    StartObservingUpgrades();
    // Synchronize the instance with the current state of detection.
    OnUpgradeRecommended();
  }
}

void RestartNotificationController::StartObservingUpgrades() {
  if (!observing_upgrades_) {
    upgrade_detector_->AddObserver(this);
    observing_upgrades_ = true;
  }
}

void RestartNotificationController::StopObservingUpgrades() {
  if (observing_upgrades_) {
    upgrade_detector_->RemoveObserver(this);
    observing_upgrades_ = false;
    last_level_ = UpgradeDetector::UPGRADE_ANNOYANCE_NONE;
  }
}

void RestartNotificationController::ShowRestartNotification() {
  DCHECK_NE(last_notification_style_, NotificationStyle::kNone);
  if (last_notification_style_ == NotificationStyle::kRecommended)
    ShowRestartRecommendedBubble();
  else
    ShowRestartRequiredDialog();
}

void RestartNotificationController::CloseRestartNotification() {}
