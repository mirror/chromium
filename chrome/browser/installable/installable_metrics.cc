// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/installable/installable_metrics.h"

#include "base/metrics/histogram_macros.h"

namespace {

void RecordMenuOpenHistogram(InstallabilityCheckStatus status) {
  UMA_HISTOGRAM_ENUMERATION("Webapp.InstallabilityCheckStatus.MenuOpen", status,
                            InstallabilityCheckStatus::COUNT);
}

void RecordMenuItemAddToHomescreenHistogram(InstallabilityCheckStatus status) {
  UMA_HISTOGRAM_ENUMERATION(
      "Webapp.InstallabilityCheckStatus.MenuItemAddToHomescreen", status,
      InstallabilityCheckStatus::COUNT);
}

void RecordAddToHomescreenHistogram(AddToHomescreenTimeoutStatus status) {
  UMA_HISTOGRAM_ENUMERATION(
      "Webapp.InstallabilityCheckStatus.AddToHomescreenTimeout", status,
      AddToHomescreenTimeoutStatus::COUNT);
}

}  // anonymous namespace

InstallableMetrics::InstallableMetrics()
    : status_(InstallabilityCheckStatus::NOT_STARTED),
      menu_open_count_(0),
      menu_item_add_to_homescreen_count_(0),
      add_to_homescreen_manifest_timeout_count_(0),
      add_to_homescreen_installability_timeout_count_(0) {}

InstallableMetrics::~InstallableMetrics() {}

void InstallableMetrics::Start() {
  if (status_ == InstallabilityCheckStatus::NOT_STARTED)
    status_ = InstallabilityCheckStatus::NOT_COMPLETED;
}

InstallabilityCheckStatus InstallableMetrics::GetStatusForTesting() const {
  return status_;
}

void InstallableMetrics::RecordMenuOpen() {
  if (is_pwa_check_complete())
    RecordMenuOpenHistogram(status_);
  else
    ++menu_open_count_;
}

void InstallableMetrics::RecordMenuItemAddToHomescreen() {
  if (is_pwa_check_complete())
    RecordMenuItemAddToHomescreenHistogram(status_);
  else
    ++menu_item_add_to_homescreen_count_;
}

void InstallableMetrics::RecordAddToHomescreenNoTimeout() {
  // For there to be no timeout, we must have successfully completed the
  // installability check, or failed early. We could still be in the
  // NOT_COMPLETED state since we don't do the full installability check if
  // WebAPKs are disabled.
  switch (status_) {
    case InstallabilityCheckStatus::COMPLETE_PROGRESSIVE_WEB_APP:
      RecordAddToHomescreenHistogram(
          AddToHomescreenTimeoutStatus::NO_TIMEOUT_PROGRESSIVE_WEB_APP);
      break;
    case InstallabilityCheckStatus::COMPLETE_NON_PROGRESSIVE_WEB_APP:
    case InstallabilityCheckStatus::NOT_COMPLETED:
      RecordAddToHomescreenHistogram(
          AddToHomescreenTimeoutStatus::NO_TIMEOUT_NON_PROGRESSIVE_WEB_APP);
      break;
    case InstallabilityCheckStatus::NOT_STARTED:
    case InstallabilityCheckStatus::IN_PROGRESS_NON_PROGRESSIVE_WEB_APP:
    case InstallabilityCheckStatus::IN_PROGRESS_PROGRESSIVE_WEB_APP:
    case InstallabilityCheckStatus::COUNT:
      NOTREACHED();
  }
}

void InstallableMetrics::RecordAddToHomescreenManifestAndIconTimeout() {
  ++add_to_homescreen_manifest_timeout_count_;
}

void InstallableMetrics::RecordAddToHomescreenInstallabilityTimeout() {
  ++add_to_homescreen_installability_timeout_count_;
}

void InstallableMetrics::RecordQueuedMetrics(bool check_passed) {
  // Don't do anything if we've already finished the PWA check.
  if (is_pwa_check_complete())
    return;

  status_ = check_passed
                ? InstallabilityCheckStatus::COMPLETE_PROGRESSIVE_WEB_APP
                : InstallabilityCheckStatus::COMPLETE_NON_PROGRESSIVE_WEB_APP;

  // Compute what the status would have been for any queued calls to
  // Record{MenuOpen,MenuItemAddToHomescreen}Histogram.
  InstallabilityCheckStatus prev_status =
      check_passed
          ? InstallabilityCheckStatus::IN_PROGRESS_PROGRESSIVE_WEB_APP
          : InstallabilityCheckStatus::IN_PROGRESS_NON_PROGRESSIVE_WEB_APP;
  for (; menu_open_count_ > 0; --menu_open_count_)
    RecordMenuOpenHistogram(prev_status);

  for (; menu_item_add_to_homescreen_count_ > 0;
       --menu_item_add_to_homescreen_count_) {
    RecordMenuItemAddToHomescreenHistogram(prev_status);
  }

  // Record the status for any queued calls to RecordAddToHomescreen*.
  auto manifest_status = AddToHomescreenTimeoutStatus::
      TIMEOUT_MANIFEST_FETCH_NON_PROGRESSIVE_WEB_APP;
  auto installability_status = AddToHomescreenTimeoutStatus::
      TIMEOUT_INSTALLABILITY_CHECK_NON_PROGRESSIVE_WEB_APP;

  if (status_ == InstallabilityCheckStatus::COMPLETE_PROGRESSIVE_WEB_APP) {
    manifest_status = AddToHomescreenTimeoutStatus::
        TIMEOUT_MANIFEST_FETCH_PROGRESSIVE_WEB_APP;
    installability_status = AddToHomescreenTimeoutStatus::
        TIMEOUT_INSTALLABILITY_CHECK_PROGRESSIVE_WEB_APP;
  }

  for (; add_to_homescreen_manifest_timeout_count_ > 0;
       --add_to_homescreen_manifest_timeout_count_) {
    RecordAddToHomescreenHistogram(manifest_status);
  }

  for (; add_to_homescreen_installability_timeout_count_ > 0;
       --add_to_homescreen_installability_timeout_count_) {
    RecordAddToHomescreenHistogram(installability_status);
  }
}

void InstallableMetrics::RecordMetricsOnNavigationAndReset(bool has_paused) {
  // We may have reset when the counts are nonzero and |status_| is one of
  // NOT_STARTED or NOT_COMPLETED. If we completed, then these values cannot be
  // anything except 0.
  InstallabilityCheckStatus status = status_;

  // If we are waiting for a service worker to appear, record this as being
  // complete and a non-PWA.
  if (has_paused && status == InstallabilityCheckStatus::NOT_COMPLETED)
    status = InstallabilityCheckStatus::COMPLETE_NON_PROGRESSIVE_WEB_APP;

  for (; menu_open_count_ > 0; --menu_open_count_)
    RecordMenuOpenHistogram(status);

  for (; menu_item_add_to_homescreen_count_ > 0;
       --menu_item_add_to_homescreen_count_) {
    RecordMenuItemAddToHomescreenHistogram(status);
  }

  for (; add_to_homescreen_manifest_timeout_count_ > 0;
       --add_to_homescreen_manifest_timeout_count_) {
    RecordAddToHomescreenHistogram(
        AddToHomescreenTimeoutStatus::TIMEOUT_MANIFEST_FETCH_UNKNOWN);
  }

  for (; add_to_homescreen_installability_timeout_count_ > 0;
       --add_to_homescreen_installability_timeout_count_) {
    RecordAddToHomescreenHistogram(
        AddToHomescreenTimeoutStatus::TIMEOUT_INSTALLABILITY_CHECK_UNKNOWN);
  }

  status_ = InstallabilityCheckStatus::NOT_STARTED;
}

bool InstallableMetrics::is_pwa_check_complete() const {
  return status_ == InstallabilityCheckStatus::COMPLETE_PROGRESSIVE_WEB_APP ||
         status_ == InstallabilityCheckStatus::COMPLETE_NON_PROGRESSIVE_WEB_APP;
}
