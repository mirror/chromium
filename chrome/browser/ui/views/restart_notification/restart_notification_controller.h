// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_RESTART_NOTIFICATION_RESTART_NOTIFICATION_CONTROLLER_H_
#define CHROME_BROWSER_UI_VIEWS_RESTART_NOTIFICATION_RESTART_NOTIFICATION_CONTROLLER_H_

#include "base/macros.h"
#include "chrome/browser/upgrade_detector.h"
#include "chrome/browser/upgrade_observer.h"
#include "components/prefs/pref_change_registrar.h"

class RestartNotificationController : public UpgradeObserver {
 public:
  explicit RestartNotificationController(UpgradeDetector* upgrade_detector);
  ~RestartNotificationController() override;

 protected:
  // UpgradeObserver:
  void OnUpgradeRecommended() override;

  // Shows the restart recommended bubble if it is not already shown.
  virtual void ShowRestartRecommendedBubble();

  // Shows the restart required dialog if it is not already shown.
  virtual void ShowRestartRequiredDialog();

 private:
  enum class NotificationStyle {
    kNone,         // No notifications are shown.
    kRecommended,  // Restarts are recommended.
    kRequired,     // Restarts are required.
  };

  // Preference observer callback function.
  void OnPreferenceChanged();

  void StartObservingUpgrades();

  void StopObservingUpgrades();

  // Shows the proper notification based on the preference setting.
  void ShowRestartNotification();

  // Closes any previously-opened notifications.
  void CloseRestartNotification();

  // The process-wide upgrade detector.
  UpgradeDetector* const upgrade_detector_;

  // Observes changes to the browser.restart_notification Local State pref.
  PrefChangeRegistrar pref_change_registrar_;

  // The last observed notification style.
  NotificationStyle last_notification_style_;

  // The last observed annoyance level for which a notification was shown.
  UpgradeDetector::UpgradeNotificationAnnoyanceLevel last_level_;

  bool observing_upgrades_;

  DISALLOW_COPY_AND_ASSIGN(RestartNotificationController);
};

#endif  // CHROME_BROWSER_UI_VIEWS_RESTART_NOTIFICATION_RESTART_NOTIFICATION_CONTROLLER_H_
