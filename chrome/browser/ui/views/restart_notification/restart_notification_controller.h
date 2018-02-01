// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_RESTART_NOTIFICATION_RESTART_NOTIFICATION_CONTROLLER_H_
#define CHROME_BROWSER_UI_VIEWS_RESTART_NOTIFICATION_RESTART_NOTIFICATION_CONTROLLER_H_

#include "base/macros.h"
#include "chrome/browser/upgrade_detector.h"
#include "chrome/browser/upgrade_observer.h"
#include "components/prefs/pref_change_registrar.h"

// A class that observes changes to the browser.restart_notification preference
// (which is backed by the RestartNotification policy setting) and annoyance
// levels from the UpgradeDetector. An appropriate notification is shown to the
// user based on the policy setting and current upgrade annoyance level.
class RestartNotificationController : public UpgradeObserver {
 public:
  // |upgrade_detector| is expected to be the process-wide detector, and must
  // outlive the controller.
  explicit RestartNotificationController(UpgradeDetector* upgrade_detector);
  ~RestartNotificationController() override;

 protected:
  // UpgradeObserver:
  void OnUpgradeRecommended() override;

 private:
  enum class NotificationStyle {
    kNone,         // No notifications are shown.
    kRecommended,  // Restarts are recommended.
    kRequired,     // Restarts are required.
  };

  // Adjusts to the current notification style as indicated by the
  // browser.restart_notification Local State preference.
  void HandleCurrentStyle();

  // Bring the instance out of or back to dormant mode.
  void StartObservingUpgrades();
  void StopObservingUpgrades();

  // Shows the proper notification based on the preference setting. Invoked as a
  // result of a detected change in the UpgradeDetector's annoyance level.
  void ShowRestartNotification();

  // Closes any previously-shown notifications. This is safe to call if no
  // notifications have been shown. Notifications may be closed by other means
  // (e.g., by the user), so there is no expectation that a previously-shown
  // notification is still open when this is invoked.
  void CloseRestartNotification();

  // The following methods, which are invoked by the controller to show or close
  // notifications, are virtual for the sake of testing.

  // Shows the restart recommended bubble if it is not already open.
  virtual void ShowRestartRecommendedBubble();

  // Closes the restart recommended bubble if it is still open.
  virtual void CloseRestartRecommendedBubble();

  // Shows the restart required dialog if it is not already open.
  virtual void ShowRestartRequiredDialog();

  // Closes the restart required dialog if it is still open.
  virtual void CloseRestartRequiredDialog();

  // The process-wide upgrade detector.
  UpgradeDetector* const upgrade_detector_;

  // Observes changes to the browser.restart_notification Local State pref.
  PrefChangeRegistrar pref_change_registrar_;

  // The last observed notification style. When kNone, the controller is
  // said to be "dormant" as there is no work for it to do aside from watch for
  // changes to browser.restart_notification. When any other value, the
  // controller is observing the UpgradeDetector to detect when to show a
  // notification.
  NotificationStyle last_notification_style_;

  // The last observed annoyance level for which a notification was shown. This
  // member is unconditionally UPGRADE_ANNOYANCE_NONE when the controller is
  // dormant (browser.restart_notification is 0). It is any other value only
  // when a notification has been shown.
  UpgradeDetector::UpgradeNotificationAnnoyanceLevel last_level_;

  DISALLOW_COPY_AND_ASSIGN(RestartNotificationController);
};

#endif  // CHROME_BROWSER_UI_VIEWS_RESTART_NOTIFICATION_RESTART_NOTIFICATION_CONTROLLER_H_
