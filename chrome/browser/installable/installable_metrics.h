// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_INSTALLABLE_INSTALLABLE_METRICS_H_
#define CHROME_BROWSER_INSTALLABLE_INSTALLABLE_METRICS_H_

#include <memory>
#include <string>

#include "chrome/browser/installable/bucket.h"

// This enum backs a UMA histogram and must be treated as append-only.
enum class InstallabilityCheckStatus {
  NOT_STARTED,
  IN_PROGRESS_UNKNOWN,
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

namespace installable {

// This contains types and helper functions for storing, incrementing
// and creating the UMA buckets that are used by InstallableManager.

// BucketSet contains a bucket for each of the histograms we care about.
struct BucketSet {
  // The following fields point to the current bucket of each of the
  // histograms.
  Bucket* menu_open_;
  Bucket* menu_item_a2hs_;
  Bucket* a2hs_no_timeout_;
  Bucket* a2hs_manifest_and_icon_timeout_;
  Bucket* a2hs_installability_timeout_;

  // Records the state of the installability check when the Android menu is
  // opened.
  void RecordMenuOpen();

  // Records the state of the installability check when the add to
  // homescreen menu item on Android is tapped.
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
};

// Constructs a menu-open Bucket which increments |status|.
std::unique_ptr<Bucket> MenuOpenBucket(InstallabilityCheckStatus status);

// Constructs a menu-open ResolvableBucket which resolves to one of
// the supplied |InstallabilityCheckStatus|es.
std::unique_ptr<ResolvableBucket> MenuOpenBucket(
    InstallabilityCheckStatus pwa_yes,
    InstallabilityCheckStatus pwa_no,
    InstallabilityCheckStatus pwa_unknown);

// Constructs a menu-item-add-to-homescreen Bucket which increments
// |status|.
std::unique_ptr<Bucket> MenuItemAddToHomescreenBucket(
    InstallabilityCheckStatus status);

// Constructs a menu-item-add-to-homescreen ResolvableBucket which
// resolves to one of the supplied |InstallabilityCheckStatus|es.
std::unique_ptr<ResolvableBucket> MenuItemAddToHomescreenBucket(
    InstallabilityCheckStatus pwa_yes,
    InstallabilityCheckStatus pwa_no,
    InstallabilityCheckStatus pwa_unknown);

// Constructs a Bucket which increments |status|.
std::unique_ptr<Bucket> AddToHomescreenTimeoutBucket(
    AddToHomescreenTimeoutStatus status);

// Constructs a ResolvableBucket which resolves to one of the supplied
// |AddToHomescreenTimeoutStatus|es.
std::unique_ptr<ResolvableBucket> AddToHomescreenTimeoutBucket(
    AddToHomescreenTimeoutStatus pwa_yes,
    AddToHomescreenTimeoutStatus pwa_no,
    AddToHomescreenTimeoutStatus pwa_unknown);

}  // namespace installable

#endif  // CHROME_BROWSER_INSTALLABLE_INSTALLABLE_METRICS_H_
