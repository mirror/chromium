// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UPGRADE_DETECTOR_H_
#define CHROME_BROWSER_UPGRADE_DETECTOR_H_

#include <memory>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "base/timer/timer.h"
#include "chrome/browser/upgrade_observer.h"
#include "ui/base/idle/idle.h"
#include "ui/gfx/image/image.h"

class PrefRegistrySimple;
class UpgradeObserver;

///////////////////////////////////////////////////////////////////////////////
// UpgradeDetector
//
// This class is a singleton class that monitors when an upgrade happens in the
// background. We basically ask Omaha what it thinks the latest version is and
// if our version is lower we send out a notification upon:
//   a) Detecting an upgrade and...
//   b) When we think the user should be notified about the upgrade.
// The latter happens much later, since we don't want to be too annoying.
//
class UpgradeDetector {
 public:
  // The Homeland Security Upgrade Advisory System.
  // These values are logged in a histogram and shouldn't be renumbered or
  // removed.
  enum UpgradeNotificationAnnoyanceLevel {
    UPGRADE_ANNOYANCE_NONE = 0,  // What? Me worry?
    UPGRADE_ANNOYANCE_LOW,       // Green.
    UPGRADE_ANNOYANCE_ELEVATED,  // Yellow.
    UPGRADE_ANNOYANCE_HIGH,      // Red.
    UPGRADE_ANNOYANCE_SEVERE,    // Orange.
    UPGRADE_ANNOYANCE_CRITICAL,  // Red exclamation mark.
    UPGRADE_ANNOYANCE_LAST = UPGRADE_ANNOYANCE_CRITICAL  // The last value
  };

  // The number of UpgradeNotificationAnnoyanceLevel enum values.
  static constexpr int kUpgradeNotificationAnnoyanceLevelCount =
      UPGRADE_ANNOYANCE_LAST + 1;

  // Creates a new instance for use in the browser process.
  static std::unique_ptr<UpgradeDetector> Create();

  virtual ~UpgradeDetector();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Whether the user should be notified about an upgrade.
  bool notify_upgrade() const {
    return upgrade_notification_stage_ != UPGRADE_ANNOYANCE_NONE;
  }

  // Whether the upgrade recommendation is due to Chrome being outdated.
  bool is_outdated_install() const {
    return (notifications_ & kOutdatedInstall) != 0;
  }

  // Returns true if auto-updates are supported but disabled.
  bool auto_updates_disabled() const { return auto_updates_disabled_; }

  // Notifify this object that the user has acknowledged the critical update
  // so we don't need to complain about it for now.
  void acknowledge_critical_update() {
    // TODO(grt): noop if policy 2 (recurring prompt).
    critical_update_acknowledged_ = true;
  }

  // Whether the user has acknowledged the critical update.
  bool critical_update_acknowledged() const {
    return critical_update_acknowledged_;
  }

  bool is_factory_reset_required() const {
    return (notifications_ & kFactoryResetRequired) != 0;
  }

  // Retrieves the right icon based on the degree of severity (see
  // UpgradeNotificationAnnoyanceLevel, each level has an an accompanying icon
  // to go with it) to display within the app menu.
  gfx::Image GetIcon();

  UpgradeNotificationAnnoyanceLevel upgrade_notification_stage() const {
    return upgrade_notification_stage_;
  }

  void AddObserver(UpgradeObserver* observer);

  void RemoveObserver(UpgradeObserver* observer);

  // Notifies that the current install is outdated. No details are expected.
  void NotifyOutdatedInstall();

  // Notifies that the current install is outdated and auto-update (AU) is
  // disabled. No details are expected.
  void NotifyOutdatedInstallNoAutoUpdate();

 protected:
  using NotificationType = uint32_t;
  enum : NotificationType {
    // A regular update is available.
    kRegularUpdateAvailable = 0x01,

    // A critical update is available.
    kCriticalUpdateAvailable = 0x02,

    // The install is outdated.
    kOutdatedInstall = 0x04,

    // An experiment has been killed and a best-effort should be made to
    // restart.
    kExperimentKillBestEffort = 0x08,

    // An experiment has been killed and a critical effort should be made to
    // restart.
    kExperimentKillCritical = 0x10,

    // A factory reset is required (Chrome OS only).
    kFactoryResetRequired = 0x20,

    // A mask of types that warrant an immediate jump to critial annoyance.
    kCriticalMask =
        (kCriticalUpdateAvailable | kOutdatedInstall | kExperimentKillCritical),
  };

  // |auto_updates_disabled| true if auto updates are supported but disabled and
  // the user should be prompted to re-enable them.
  explicit UpgradeDetector(bool auto_updates_disabled);

  // Notifies that update is recommended and triggers different actions based
  // on the update availability.
  void NotifyUpgrade(UpgradeNotificationAnnoyanceLevel level);

  // Notifies that update is recommended.
  void NotifyUpgradeRecommended();

  // Notifies that a critical update has been installed. No details are
  // expected.
  void NotifyCriticalUpgradeInstalled();

  // The function that sends out a notification that lets the rest of the UI
  // know we should notify the user that a new update is available to download
  // over cellular connection.
  void NotifyUpdateOverCellularAvailable();

  // Notifies that the user's one time permission on update over cellular
  // connection has been granted.
  void NotifyUpdateOverCellularOneTimePermissionGranted();

  // Triggers a critical update, which starts a timer that checks the machine
  // idle state. Protected and virtual so that it could be overridden by tests.
  virtual void TriggerCriticalUpdate();

  bool has_critical_notifications() const {
    return (notifications_ & kCriticalMask) != 0;
  }

  bool critical_experiment_updates_available() const {
    return (notifications_ & kExperimentKillCritical) != 0;
  }

  void set_critical_update_acknowledged(bool acknowledged) {
    critical_update_acknowledged_ = acknowledged;
  }

  void set_is_factory_reset_required(bool is_factory_reset_required) {
    if (is_factory_reset_required)
      notifications_ |= kFactoryResetRequired;
    else
      notifications_ &= ~kFactoryResetRequired;
  }

  void add_notifications(NotificationType notifications) {
    notifications_ |= notifications;
  }

 private:
  FRIEND_TEST_ALL_PREFIXES(AppMenuModelTest, Basics);
  FRIEND_TEST_ALL_PREFIXES(SystemTrayClientTest, UpdateTrayIcon);
  friend class UpgradeMetricsProviderTest;

  // Initiates an Idle check. See IdleCallback below.
  void CheckIdle();

  // The callback for the IdleCheck. Tells us whether Chrome has received any
  // input events since the specified time.
  void IdleCallback(ui::IdleState state);

  // Whether any software updates are available (experiment updates are tracked
  // separately via additional member variables below).
  NotificationType notifications_;

  // True if this browser is integrated with an auto-updater but such updates
  // are disabled.
  const bool auto_updates_disabled_;

  // Whether the user has acknowledged the critical update.
  bool critical_update_acknowledged_;

  // A timer to check to see if we've been idle for long enough to show the
  // critical warning. Should only be set if |upgrade_available_| is
  // UPGRADE_AVAILABLE_CRITICAL.
  base::RepeatingTimer idle_check_timer_;

  // The stage at which the annoyance level for upgrade notifications is at.
  UpgradeNotificationAnnoyanceLevel upgrade_notification_stage_;

  base::ObserverList<UpgradeObserver> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(UpgradeDetector);
};

#endif  // CHROME_BROWSER_UPGRADE_DETECTOR_H_
