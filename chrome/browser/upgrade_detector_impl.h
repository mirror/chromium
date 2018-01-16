// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UPGRADE_DETECTOR_IMPL_H_
#define CHROME_BROWSER_UPGRADE_DETECTOR_IMPL_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/timer/timer.h"
#include "base/version.h"
#include "build/build_config.h"
#include "chrome/browser/upgrade_detector.h"
#include "components/variations/service/variations_service.h"

namespace base {
class SequencedTaskRunner;
}

// A concrete UpgradeDetector for desktop Chrome (Windows, Mac, Linux).
//
// This detector monitors the following to determine if a browser restart is
// needed:
// - The install location, in case a newer version of the browser is
//   installed while the current one is running.
// - The VariationsService, in case an experiment kill switch is activated.
// - A trusted time source, in case the current browser was built sufficiently
//   long ago that an update should have been applied.
class UpgradeDetectorImpl : public UpgradeDetector,
                            public variations::VariationsService::Observer {
 public:
  ~UpgradeDetectorImpl() override;

  // Returns the currently installed Chrome version, which may be newer than the
  // one currently running. Not supported on Android, iOS or ChromeOS. Must be
  // run on a thread where I/O operations are allowed.
  static base::Version GetCurrentlyInstalledVersion();

 protected:
  UpgradeDetectorImpl();

  // variations::VariationsService::Observer:
  void OnExperimentChangesDetected(Severity severity) override;

  // Trigger an "on upgrade" notification based on the specified |time_passed|
  // interval. Exposed as protected for testing.
  void NotifyOnUpgradeWithTimePassed(base::TimeDelta time_passed);

 private:
  friend class UpgradeDetector;

  // Start the timer that will call |CheckForUpgrade()|.
  void StartTimerForUpgradeCheck();

  // Launches a background task to check if we have the latest version.
  void CheckForUpgrade();

  // Starts the upgrade notification timer that will check periodically whether
  // enough time has elapsed to update the severity (which maps to visual
  // badging) of the notification.
  void StartUpgradeNotificationTimer();

  // Returns true after calling UpgradeDetected if current install is outdated.
  bool DetectOutdatedInstall();

  // The function that sends out a notification (after a certain time has
  // elapsed) that lets the rest of the UI know we should start notifying the
  // user that a new version is available.
  void NotifyOnUpgrade();

  void OnUpgradeDetected(bool is_critical);

  // Sends out a notification and starts a one shot timer to wait until
  // notifying the user.
  void ReceivedNotification(NotificationType notification);

  SEQUENCE_CHECKER(sequence_checker_);

  // A sequenced task runner on which blocking tasks run.
  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;

  // We periodically check to see if Chrome has been upgraded.
  base::RepeatingTimer detect_upgrade_timer_;

  // After we detect an upgrade we start a recurring timer to see if enough time
  // has passed and we should start notifying the user.
  base::RepeatingTimer upgrade_notification_timer_;

  // True if this browser is on the Google Chrome canary or dev channels.
  const bool is_unstable_channel_;

  // When the upgrade was detected - either a software update or a variations
  // update, whichever happened first.
  base::TimeTicks upgrade_detected_time_;

  // The date the binaries were built.
  base::Time build_date_;

  // We use this factory to create callback tasks for UpgradeDetected. We pass
  // the task to the actual upgrade detection code, which is in
  // DetectUpgradeTask.
  base::WeakPtrFactory<UpgradeDetectorImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(UpgradeDetectorImpl);
};

#endif  // CHROME_BROWSER_UPGRADE_DETECTOR_IMPL_H_
