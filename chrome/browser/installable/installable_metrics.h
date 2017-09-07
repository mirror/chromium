// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_INSTALLABLE_INSTALLABLE_METRICS_H_
#define CHROME_BROWSER_INSTALLABLE_INSTALLABLE_METRICS_H_

// This enum backs a UMA histogram and must be treated as append-only.
enum class InstallabilityCheckStatus {
  NOT_STARTED,
  NOT_COMPLETED,
  IN_PROGRESS_NON_PROGRESSIVE_WEB_APP,
  IN_PROGRESS_PROGRESSIVE_WEB_APP,
  COMPLETE_NON_PROGRESSIVE_WEB_APP,
  COMPLETE_PROGRESSIVE_WEB_APP,
  COUNT,
};

// This enum backs a UMA histogram and must be treated as append-only.
enum class AddToHomescreenTimeoutStatus {
  NO_TIMEOUT_NON_PROGRESSIVE_WEB_APP,
  NO_TIMEOUT_PROGRESSIVE_WEB_APP,
  TIMEOUT_MANIFEST_FETCH_NON_PROGRESSIVE_WEB_APP,
  TIMEOUT_MANIFEST_FETCH_PROGRESSIVE_WEB_APP,
  TIMEOUT_MANIFEST_FETCH_UNKNOWN,
  TIMEOUT_INSTALLABILITY_CHECK_NON_PROGRESSIVE_WEB_APP,
  TIMEOUT_INSTALLABILITY_CHECK_PROGRESSIVE_WEB_APP,
  TIMEOUT_INSTALLABILITY_CHECK_UNKNOWN,
  COUNT,
};

class InstallableMetrics {
 public:
  InstallableMetrics();
  ~InstallableMetrics();

  // Called to record the opening of the Android menu.
  void RecordMenuOpen();

  // Called to record a tap on the add to homescreen menu item on Android.
  void RecordMenuItemAddToHomescreen();

  // Called to record the add to homescreen dialog not timing out on the
  // InstallableManager work.
  void RecordAddToHomescreenNoTimeout();

  // Called to record the add to homescreen dialog timing out during the
  // manifest and icon fetch.
  void RecordAddToHomescreenManifestAndIconTimeout();

  // Called to record the add to homescreen dialog timing out on the service
  // worker + installability check.
  void RecordAddToHomescreenInstallabilityTimeout();

  // Called to save any queued metrics from incomplete tasks.
  void RecordQueuedMetrics(bool check_passed);

  // Called to save any queued metrics at the time of navigation, and to reset
  // state.
  void RecordMetricsOnNavigationAndReset();

  // Called to indicate that the InstallableManager has started working on the
  // current page.
  void Start();

  InstallabilityCheckStatus GetStatusForTesting() const;

 private:
  void RecordMetricsAndResetCounts(
      InstallabilityCheckStatus status,
      AddToHomescreenTimeoutStatus manifest_status,
      AddToHomescreenTimeoutStatus installability_status);

  // Returns true if we have finished the checking for the current page.
  bool is_pwa_check_complete() const;

  // Whether or not the current page is a PWA.
  InstallabilityCheckStatus status_;

  // Counts for the number of queued requests of the menu and add to homescreen
  // menu item there have been whilst the installable check is running.
  int menu_open_count_;
  int menu_item_add_to_homescreen_count_;

  // Counts for the number of times the add to homescreen dialog times out at
  // different stages of the installable check.
  int add_to_homescreen_manifest_timeout_count_;
  int add_to_homescreen_installability_timeout_count_;
};

#endif  // CHROME_BROWSER_INSTALLABLE_INSTALLABLE_METRICS_H_
