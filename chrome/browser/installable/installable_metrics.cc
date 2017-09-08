// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/installable/installable_metrics.h"

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"

namespace {

void WriteMenuOpenHistogram(InstallabilityCheckStatus status, int count) {
  for (int i = 0; i < count; ++i) {
    UMA_HISTOGRAM_ENUMERATION("Webapp.InstallabilityCheckStatus.MenuOpen",
                              status, InstallabilityCheckStatus::COUNT);
  }
}

void WriteMenuItemAddToHomescreenHistogram(InstallabilityCheckStatus status,
                                           int count) {
  for (int i = 0; i < count; ++i) {
    UMA_HISTOGRAM_ENUMERATION(
        "Webapp.InstallabilityCheckStatus.MenuItemAddToHomescreen", status,
        InstallabilityCheckStatus::COUNT);
  }
}

void WriteAddToHomescreenHistogram(AddToHomescreenTimeoutStatus status,
                                   int count) {
  for (int i = 0; i < count; ++i) {
    UMA_HISTOGRAM_ENUMERATION(
        "Webapp.InstallabilityCheckStatus.AddToHomescreenTimeout", status,
        AddToHomescreenTimeoutStatus::COUNT);
  }
}

class AccumulatingRecorder : public InstallableMetrics::Recorder {
 public:
  AccumulatingRecorder()
      : InstallableMetrics::Recorder(),
        menu_open_count_(0),
        menu_item_add_to_homescreen_count_(0),
        add_to_homescreen_manifest_timeout_count_(0),
        add_to_homescreen_installability_timeout_count_(0),
        started_(false) {}

  ~AccumulatingRecorder() override {
    DCHECK_EQ(0, menu_open_count_);
    DCHECK_EQ(0, menu_item_add_to_homescreen_count_);
    DCHECK_EQ(0, add_to_homescreen_manifest_timeout_count_);
    DCHECK_EQ(0, add_to_homescreen_installability_timeout_count_);
  }

  void Resolve(bool check_passed) override {
    // Resolve queued metrics to their appropriate state based on whether or not
    // we passed the installability check.
    InstallabilityCheckStatus status =
        check_passed
            ? InstallabilityCheckStatus::IN_PROGRESS_PROGRESSIVE_WEB_APP
            : InstallabilityCheckStatus::IN_PROGRESS_NON_PROGRESSIVE_WEB_APP;

    auto manifest_status =
        check_passed ? AddToHomescreenTimeoutStatus::
                           TIMEOUT_MANIFEST_FETCH_PROGRESSIVE_WEB_APP
                     : AddToHomescreenTimeoutStatus::
                           TIMEOUT_MANIFEST_FETCH_NON_PROGRESSIVE_WEB_APP;

    auto installability_status =
        check_passed ? AddToHomescreenTimeoutStatus::
                           TIMEOUT_INSTALLABILITY_CHECK_PROGRESSIVE_WEB_APP
                     : AddToHomescreenTimeoutStatus::
                           TIMEOUT_INSTALLABILITY_CHECK_NON_PROGRESSIVE_WEB_APP;

    WriteMetricsAndResetCounts(status, manifest_status, installability_status);
  }

  void Flush(bool has_paused) override {
    InstallabilityCheckStatus status =
        started_ ? InstallabilityCheckStatus::NOT_COMPLETED
                 : InstallabilityCheckStatus::NOT_STARTED;

    // If we have paused tasks, we are waiting for a service worker so it is
    // likely that this is not a PWA.
    if (has_paused)
      status = InstallabilityCheckStatus::COMPLETE_NON_PROGRESSIVE_WEB_APP;

    WriteMetricsAndResetCounts(
        status, AddToHomescreenTimeoutStatus::TIMEOUT_MANIFEST_FETCH_UNKNOWN,
        AddToHomescreenTimeoutStatus::TIMEOUT_INSTALLABILITY_CHECK_UNKNOWN);
  }

  void RecordMenuOpen() override { ++menu_open_count_; }

  void RecordMenuItemAddToHomescreen() override {
    ++menu_item_add_to_homescreen_count_;
  }

  void RecordAddToHomescreenNoTimeout() override {
    WriteAddToHomescreenHistogram(
        AddToHomescreenTimeoutStatus::NO_TIMEOUT_NON_PROGRESSIVE_WEB_APP, 1);
  }

  void RecordAddToHomescreenManifestAndIconTimeout() override {
    ++add_to_homescreen_manifest_timeout_count_;
  }

  void RecordAddToHomescreenInstallabilityTimeout() override {
    ++add_to_homescreen_installability_timeout_count_;
  }

  void Start() override { started_ = true; }

 private:
  void WriteMetricsAndResetCounts(
      InstallabilityCheckStatus status,
      AddToHomescreenTimeoutStatus manifest_status,
      AddToHomescreenTimeoutStatus installability_status) {
    WriteMenuOpenHistogram(status, menu_open_count_);
    WriteMenuItemAddToHomescreenHistogram(status,
                                          menu_item_add_to_homescreen_count_);
    WriteAddToHomescreenHistogram(manifest_status,
                                  add_to_homescreen_manifest_timeout_count_);
    WriteAddToHomescreenHistogram(
        installability_status, add_to_homescreen_installability_timeout_count_);

    menu_open_count_ = 0;
    menu_item_add_to_homescreen_count_ = 0;
    add_to_homescreen_manifest_timeout_count_ = 0;
    add_to_homescreen_installability_timeout_count_ = 0;
  }

  // Counts for the number of queued requests of the menu and add to homescreen
  // menu item there have been whilst the installable check is running.
  int menu_open_count_;
  int menu_item_add_to_homescreen_count_;

  // Counts for the number of times the add to homescreen dialog times out at
  // different stages of the installable check.
  int add_to_homescreen_manifest_timeout_count_;
  int add_to_homescreen_installability_timeout_count_;

  // True if we have started working yet.
  bool started_;
};

class DirectRecorder : public InstallableMetrics::Recorder {
 public:
  explicit DirectRecorder(bool check_passed)
      : InstallableMetrics::Recorder(),
        status_(
            check_passed
                ? InstallabilityCheckStatus::COMPLETE_PROGRESSIVE_WEB_APP
                : InstallabilityCheckStatus::COMPLETE_NON_PROGRESSIVE_WEB_APP) {
  }

  ~DirectRecorder() override {}

  void Resolve(bool check_passed) override {}
  void Flush(bool has_paused) override {}
  void Start() override {}
  void RecordMenuOpen() override { WriteMenuOpenHistogram(status_, 1); }

  void RecordMenuItemAddToHomescreen() override {
    WriteMenuItemAddToHomescreenHistogram(status_, 1);
  }

  void RecordAddToHomescreenNoTimeout() override {
    WriteAddToHomescreenHistogram(GetNoTimeoutStatus(), 1);
  }

  void RecordAddToHomescreenManifestAndIconTimeout() override {
    WriteAddToHomescreenHistogram(GetManifestAndIconTimeoutStatus(), 1);
  }

  void RecordAddToHomescreenInstallabilityTimeout() override {
    WriteAddToHomescreenHistogram(GetInstallabilityTimeoutStatus(), 1);
  }

 private:
  AddToHomescreenTimeoutStatus GetNoTimeoutStatus() const {
    switch (status_) {
      case InstallabilityCheckStatus::COMPLETE_PROGRESSIVE_WEB_APP:
        return AddToHomescreenTimeoutStatus::NO_TIMEOUT_PROGRESSIVE_WEB_APP;
      case InstallabilityCheckStatus::COMPLETE_NON_PROGRESSIVE_WEB_APP:
        return AddToHomescreenTimeoutStatus::NO_TIMEOUT_NON_PROGRESSIVE_WEB_APP;
      case InstallabilityCheckStatus::NOT_COMPLETED:
      case InstallabilityCheckStatus::NOT_STARTED:
      case InstallabilityCheckStatus::IN_PROGRESS_NON_PROGRESSIVE_WEB_APP:
      case InstallabilityCheckStatus::IN_PROGRESS_PROGRESSIVE_WEB_APP:
      case InstallabilityCheckStatus::COUNT:
        NOTREACHED();
    }
    return AddToHomescreenTimeoutStatus::COUNT;
  }

  AddToHomescreenTimeoutStatus GetManifestAndIconTimeoutStatus() const {
    switch (status_) {
      case InstallabilityCheckStatus::COMPLETE_PROGRESSIVE_WEB_APP:
        return AddToHomescreenTimeoutStatus::
            TIMEOUT_MANIFEST_FETCH_PROGRESSIVE_WEB_APP;
      case InstallabilityCheckStatus::COMPLETE_NON_PROGRESSIVE_WEB_APP:
        return AddToHomescreenTimeoutStatus::
            TIMEOUT_MANIFEST_FETCH_NON_PROGRESSIVE_WEB_APP;
      case InstallabilityCheckStatus::NOT_COMPLETED:
      case InstallabilityCheckStatus::NOT_STARTED:
      case InstallabilityCheckStatus::IN_PROGRESS_NON_PROGRESSIVE_WEB_APP:
      case InstallabilityCheckStatus::IN_PROGRESS_PROGRESSIVE_WEB_APP:
      case InstallabilityCheckStatus::COUNT:
        NOTREACHED();
    }
    return AddToHomescreenTimeoutStatus::COUNT;
  }

  AddToHomescreenTimeoutStatus GetInstallabilityTimeoutStatus() const {
    switch (status_) {
      case InstallabilityCheckStatus::COMPLETE_PROGRESSIVE_WEB_APP:
        return AddToHomescreenTimeoutStatus::
            TIMEOUT_INSTALLABILITY_CHECK_PROGRESSIVE_WEB_APP;
      case InstallabilityCheckStatus::COMPLETE_NON_PROGRESSIVE_WEB_APP:
        return AddToHomescreenTimeoutStatus::
            TIMEOUT_INSTALLABILITY_CHECK_NON_PROGRESSIVE_WEB_APP;
      case InstallabilityCheckStatus::NOT_COMPLETED:
      case InstallabilityCheckStatus::NOT_STARTED:
      case InstallabilityCheckStatus::IN_PROGRESS_NON_PROGRESSIVE_WEB_APP:
      case InstallabilityCheckStatus::IN_PROGRESS_PROGRESSIVE_WEB_APP:
      case InstallabilityCheckStatus::COUNT:
        NOTREACHED();
    }
    return AddToHomescreenTimeoutStatus::COUNT;
  }

  InstallabilityCheckStatus status_;
};

}  // anonymous namespace

InstallableMetrics::InstallableMetrics()
    : recorder_(base::MakeUnique<AccumulatingRecorder>()) {}

InstallableMetrics::~InstallableMetrics() {}

void InstallableMetrics::RecordMenuOpen() {
  recorder_->RecordMenuOpen();
}

void InstallableMetrics::RecordMenuItemAddToHomescreen() {
  recorder_->RecordMenuItemAddToHomescreen();
}

void InstallableMetrics::RecordAddToHomescreenNoTimeout() {
  recorder_->RecordAddToHomescreenNoTimeout();
}

void InstallableMetrics::RecordAddToHomescreenManifestAndIconTimeout() {
  recorder_->RecordAddToHomescreenManifestAndIconTimeout();
}

void InstallableMetrics::RecordAddToHomescreenInstallabilityTimeout() {
  recorder_->RecordAddToHomescreenInstallabilityTimeout();
}

void InstallableMetrics::Resolve(bool check_passed) {
  recorder_->Resolve(check_passed);
  recorder_ = base::MakeUnique<DirectRecorder>(check_passed);
}

void InstallableMetrics::Start() {
  recorder_->Start();
}

void InstallableMetrics::Flush(bool has_paused) {
  recorder_->Flush(has_paused);
  recorder_ = base::MakeUnique<AccumulatingRecorder>();
}
