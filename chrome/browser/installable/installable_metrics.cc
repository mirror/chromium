// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/installable/installable_metrics.h"

#include "base/metrics/histogram_macros.h"
#include "chrome/browser/installable/bucket.h"

namespace installable {

// A Bucket in a histogram with InstallabilityCheckStatus elements.
class InstallabilityCheckBucket : public Bucket {
 public:
  InstallabilityCheckBucket(const std::string& hist_name,
                            InstallabilityCheckStatus status)
      : hist_name_(hist_name), status_(status) {}

  void Increment() override {
    UMA_HISTOGRAM_ENUMERATION(hist_name_, status_,
                              InstallabilityCheckStatus::COUNT);
  }

 private:
  std::string hist_name_;
  InstallabilityCheckStatus status_;
};

// A Bucket in the Webapp.InstallabilityCheckStatus.AddToHomescreenTimeout
// histogram.
class A2HSTimeoutBucket : public Bucket {
 public:
  explicit A2HSTimeoutBucket(AddToHomescreenTimeoutStatus status)
      : status_(status) {}

  void Increment() override {
    UMA_HISTOGRAM_ENUMERATION(
        "Webapp.InstallabilityCheckStatus.AddToHomescreenTimeout", status_,
        AddToHomescreenTimeoutStatus::COUNT);
  }

 private:
  AddToHomescreenTimeoutStatus status_;
};

void BucketSet::RecordMenuOpen() {
  menu_open_->Increment();
}

void BucketSet::RecordMenuItemAddToHomescreen() {
  menu_item_a2hs_->Increment();
}

void BucketSet::RecordAddToHomescreenNoTimeout() {
  a2hs_no_timeout_->Increment();
}

void BucketSet::RecordAddToHomescreenManifestAndIconTimeout() {
  a2hs_manifest_and_icon_timeout_->Increment();
}

void BucketSet::RecordAddToHomescreenInstallabilityTimeout() {
  a2hs_installability_timeout_->Increment();
}

std::unique_ptr<Bucket> MenuOpenBucket(InstallabilityCheckStatus status) {
  return base::MakeUnique<InstallabilityCheckBucket>(
      "Webapp.InstallabilityCheckStatus.MenuOpen", status);
}

std::unique_ptr<ResolvableBucket> MenuOpenBucket(
    InstallabilityCheckStatus pwa_yes,
    InstallabilityCheckStatus pwa_no,
    InstallabilityCheckStatus pwa_unknown) {
  return base::MakeUnique<ResolvableBucket>(
      std::unique_ptr<Bucket>(MenuOpenBucket(pwa_yes)),
      std::unique_ptr<Bucket>(MenuOpenBucket(pwa_no)),
      std::unique_ptr<Bucket>(MenuOpenBucket(pwa_unknown)));
}

std::unique_ptr<Bucket> MenuItemAddToHomescreenBucket(
    InstallabilityCheckStatus status) {
  return base::MakeUnique<InstallabilityCheckBucket>(
      "Webapp.InstallabilityCheckStatus.MenuItemAddToHomescreen", status);
}

std::unique_ptr<ResolvableBucket> MenuItemAddToHomescreenBucket(
    InstallabilityCheckStatus pwa_yes,
    InstallabilityCheckStatus pwa_no,
    InstallabilityCheckStatus pwa_unknown) {
  return base::MakeUnique<ResolvableBucket>(
      MenuItemAddToHomescreenBucket(pwa_yes),
      MenuItemAddToHomescreenBucket(pwa_no),
      MenuItemAddToHomescreenBucket(pwa_unknown));
}

std::unique_ptr<Bucket> AddToHomescreenTimeoutBucket(
    AddToHomescreenTimeoutStatus status) {
  return base::MakeUnique<A2HSTimeoutBucket>(status);
}

std::unique_ptr<ResolvableBucket> AddToHomescreenTimeoutBucket(
    AddToHomescreenTimeoutStatus pwa_yes,
    AddToHomescreenTimeoutStatus pwa_no,
    AddToHomescreenTimeoutStatus pwa_unknown) {
  return base::MakeUnique<ResolvableBucket>(
      AddToHomescreenTimeoutBucket(pwa_yes),
      AddToHomescreenTimeoutBucket(pwa_no),
      AddToHomescreenTimeoutBucket(pwa_unknown));
}

}  // namespace installable
