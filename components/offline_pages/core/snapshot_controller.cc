// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/snapshot_controller.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "components/offline_pages/core/offline_page_feature.h"

namespace {
const bool kDocumentAvailableTriggersSnapshot = true;

// Fraction of images that need to arrive before we consider the page "done".
// This is chosen to be fairly high, so we can wait for that one last image on
// a page with lots of images, or we get all the images for pages with fewer
// images.  It will be tuned by experiment before enabling the resource based
// snapshot flag.
const float kSnapshotTriggerFraction = 0.94f;

// Default delay, in milliseconds, between the main document parsed event and
// snapshot. Note: this snapshot might not occur if the OnLoad event and
// OnLoad delay elapses first to trigger a final snapshot.
const int64_t kDefaultDelayAfterDocumentAvailableMs = 7000;

// Default delay, in milliseconds, between the main document OnLoad event and
// snapshot.
const int64_t kDelayAfterDocumentOnLoadCompletedMsForeground = 1000;
const int64_t kDelayAfterDocumentOnLoadCompletedMsBackground = 2000;

// Default delay, in milliseconds, between renovations finishing and
// taking a snapshot. Allows for page to update in response to the
// renovations.
const int64_t kDelayAfterRenovationsCompletedMs = 2000;

// Delay for testing to keep polling times reasonable.
const int64_t kDelayForTests = 0;

}  // namespace

namespace offline_pages {

// static
std::unique_ptr<SnapshotController>
SnapshotController::CreateForForegroundOfflining(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    SnapshotController::Client* client) {
  return std::unique_ptr<SnapshotController>(new SnapshotController(
      task_runner, client, kDefaultDelayAfterDocumentAvailableMs,
      kDelayAfterDocumentOnLoadCompletedMsForeground,
      kDelayAfterRenovationsCompletedMs, kDocumentAvailableTriggersSnapshot,
      false));
}

// static
std::unique_ptr<SnapshotController>
SnapshotController::CreateForBackgroundOfflining(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    SnapshotController::Client* client,
    bool renovations_enabled) {
  return std::unique_ptr<SnapshotController>(new SnapshotController(
      task_runner, client, kDefaultDelayAfterDocumentAvailableMs,
      kDelayAfterDocumentOnLoadCompletedMsBackground,
      kDelayAfterRenovationsCompletedMs, !kDocumentAvailableTriggersSnapshot,
      renovations_enabled));
}

SnapshotController::SnapshotController(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    SnapshotController::Client* client,
    int64_t delay_after_document_available_ms,
    int64_t delay_after_document_on_load_completed_ms,
    int64_t delay_after_renovations_completed_ms,
    bool document_available_triggers_snapshot,
    bool renovations_enabled)
    : task_runner_(task_runner),
      client_(client),
      state_(State::READY),
      delay_after_document_available_ms_(delay_after_document_available_ms),
      delay_after_document_on_load_completed_ms_(
          delay_after_document_on_load_completed_ms),
      delay_after_renovations_completed_ms_(
          delay_after_renovations_completed_ms),
      document_available_triggers_snapshot_(
          document_available_triggers_snapshot),
      loaded_error_page_(false),
      renovations_enabled_(renovations_enabled),
      renovations_completed_(false),
      images_requested_(0),
      images_completed_(0),
      css_requested_(0),
      css_completed_(0),
      weak_ptr_factory_(this) {
  if (offline_pages::ShouldUseTestingSnapshotDelay()) {
    delay_after_document_available_ms_ = kDelayForTests;
    delay_after_document_on_load_completed_ms_ = kDelayForTests;
    delay_after_renovations_completed_ms_ = kDelayForTests;
  }
}

SnapshotController::~SnapshotController() {}

void SnapshotController::Reset() {
  // Cancel potentially delayed tasks that relate to the previous 'session'.
  weak_ptr_factory_.InvalidateWeakPtrs();
  state_ = State::READY;
  current_page_quality_ = PageQuality::POOR;
  loaded_error_page_ = false;
}

void SnapshotController::Stop() {
  state_ = State::STOPPED;
}

void SnapshotController::LoadedErrorPage() {
  current_page_quality_ = PageQuality::POOR;
  loaded_error_page_ = true;
}

void SnapshotController::PendingSnapshotCompleted() {
  // Unless the controller is "stopped", enable the subsequent snapshots.
  // Stopped state prevents any further snapshots form being started.
  if (state_ == State::STOPPED)
    return;
  state_ = State::READY;
}

void SnapshotController::RenovationsCompleted() {
  if (renovations_enabled_) {
    renovations_completed_ = true;
    // If the page is not yet at sufficient quality, wait for it.  Otherwise
    // take a snapshot.
    if (current_page_quality_ < PageQuality::HIGH)
      return;

    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&SnapshotController::MaybeStartSnapshotThenStop,
                   weak_ptr_factory_.GetWeakPtr()),
        base::TimeDelta::FromMilliseconds(
            delay_after_renovations_completed_ms_));
  }
}

void SnapshotController::DocumentAvailableInMainFrame() {
  // Mark the low bar set.
  if (current_page_quality_ < PageQuality::FAIR_AND_IMPROVING &&
      !loaded_error_page_)
    current_page_quality_ = PageQuality::FAIR_AND_IMPROVING;

  if (document_available_triggers_snapshot_) {
    // Post a delayed task to snapshot.
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&SnapshotController::MaybeStartSnapshot,
                   weak_ptr_factory_.GetWeakPtr()),
        base::TimeDelta::FromMilliseconds(delay_after_document_available_ms_));
  }
}

void SnapshotController::DocumentOnLoadCompletedInMainFrame() {
  // Mark the "medium bar" set.
  if (current_page_quality_ < PageQuality::GOOD && !loaded_error_page_)
    current_page_quality_ = PageQuality::GOOD;

  if (renovations_enabled_) {
    // Run renovations. After renovations complete, a snapshot will be
    // triggered after a delay.
    client_->RunRenovations();
    // TODO(petewil): Someday we may add renovations that cause more image
    // loading - we can use the resource percentage signal.  We may need
    // to reset the page quality in case all the known images have loaded,
    // but the renovation makes more images available.
  } else if (IsOfflinePagesResourceBasedSnapshotEnabled()) {
    // If we are using the resource percentage signal, check that the resource
    // percentage has reached the threshold before snapshotting.
    if (ProgressIsGoodEnoughForSnapshot()) {
      // Post a delayed task to snapshot and then stop this controller.
      task_runner_->PostDelayedTask(
          FROM_HERE,
          base::Bind(&SnapshotController::MaybeStartSnapshotThenStop,
                     weak_ptr_factory_.GetWeakPtr()),
          base::TimeDelta::FromMilliseconds(
              delay_after_document_on_load_completed_ms_));
    }
  } else {
    // If we are not using the resource percentage signal, take a snapshot.
    // Post a delayed task to snapshot and then stop this controller.
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&SnapshotController::MaybeStartSnapshotThenStop,
                   weak_ptr_factory_.GetWeakPtr()),
        base::TimeDelta::FromMilliseconds(
            delay_after_document_on_load_completed_ms_));
  }
}

void SnapshotController::MaybeStartSnapshot() {
  if (state_ != State::READY)
    return;
  state_ = State::SNAPSHOT_PENDING;
  client_->StartSnapshot();
}

void SnapshotController::MaybeStartSnapshotThenStop() {
  MaybeStartSnapshot();
  Stop();
}

int64_t SnapshotController::GetDelayAfterDocumentAvailableForTest() {
  return delay_after_document_available_ms_;
}

int64_t SnapshotController::GetDelayAfterDocumentOnLoadCompletedForTest() {
  return delay_after_document_on_load_completed_ms_;
}

int64_t SnapshotController::GetDelayAfterRenovationsCompletedForTest() {
  return delay_after_renovations_completed_ms_;
}

// A more complex alternative for Right Moment Detection (RMD).  Instead of just
// waiting for a timer after a load event, use signals to check the load
// progress, and snapshot when the page looks mostly complete.
void SnapshotController::UpdateLoadingResourceProgress(int images_requested,
                                                       int images_completed,
                                                       int css_requested,
                                                       int css_completed) {
  // Save off progress so far
  images_requested_ = images_requested;
  images_completed_ = images_completed;
  css_requested_ = css_requested;
  css_completed_ = css_completed;

  // If all the conditions are met, take a snapshot.  Page quality is now as
  // good as we think it will get.  Post a task to snapshot and then stop this
  // controller.
  if (ProgressIsGoodEnoughForSnapshot()) {
    task_runner_->PostTask(
        FROM_HERE, base::Bind(&SnapshotController::MaybeStartSnapshotThenStop,
                              weak_ptr_factory_.GetWeakPtr()));
  }
}

bool SnapshotController::ProgressIsGoodEnoughForSnapshot() {
  // Ensure we have met at least the low bar.
  if (current_page_quality_ < PageQuality::FAIR_AND_IMPROVING) {
    return false;
  }

  // If we have already triggered a snapshot, don't trigger again.
  if (current_page_quality_ == PageQuality::HIGH)
    return false;

  // TODO(petewil): What happens if the page loads no resources? Then we wait
  // for load complete.
  // TODO(petewil): What happens if resources arrive before the DOM load event?
  // Then we wait.

  // We want all CSS to have arrived.
  if (css_requested_ > css_completed_)
    return false;

  // We want a high percentage of images.
  if (images_requested_ > 0 &&
      (kSnapshotTriggerFraction >
       (float)images_completed_ / (float)images_requested_))
    return false;

  // Set page quality to the highest value if we pass the tests.
  current_page_quality_ = PageQuality::HIGH;

  // If we are waiting for renovations, we aren't done yet.
  if (renovations_enabled_ && !renovations_completed_)
    return false;

  return true;
}

}  // namespace offline_pages
