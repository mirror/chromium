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

  void RecordMenuOpen();
  void RecordMenuItemAddToHomescreen();
  void RecordAddToHomescreenNoTimeout();
  void RecordAddToHomescreenManifestAndIconTimeout();
  void RecordAddToHomescreenInstallabilityTimeout();
  void RecordQueuedMetrics(bool check_passed);

  void RecordMetricsOnNavigationAndReset();

  void Start();
  InstallabilityCheckStatus GetStatusForTesting() const;

 private:
  bool is_pwa_check_complete() const;

  // Whether or not the current page is a PWA.
  InstallabilityCheckStatus status_;

  // Counts for the number of queued requests of the menu and add to homescreen
  // menu item there have been whilst the installable check is awaiting
  // completion. Used for metrics recording.
  int menu_open_count_;
  int menu_item_add_to_homescreen_count_;

  // Counts for the number of times the add to homescreen dialog times out at
  // different stages of the installable check.
  int add_to_homescreen_manifest_timeout_count_;
  int add_to_homescreen_installability_timeout_count_;
};

#endif  // CHROME_BROWSER_INSTALLABLE_INSTALLABLE_METRICS_H_
