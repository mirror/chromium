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
const float kSnapshotTriggerFraction = 0.97;

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
      renovations_enabled_(renovations_enabled),
      renovations_completed_(false),
      low_bar_met_(false),
      medium_bar_met_(false),
      high_bar_met_(false),
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
}

void SnapshotController::Stop() {
  state_ = State::STOPPED;
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
  if (document_available_triggers_snapshot_) {
    // TODO: Do I have to only set this member variable on the main thread?
    low_bar_met_ = true;
    // Post a delayed task to snapshot.
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&SnapshotController::MaybeStartSnapshot,
                   weak_ptr_factory_.GetWeakPtr()),
        base::TimeDelta::FromMilliseconds(delay_after_document_available_ms_));
  }
}

void SnapshotController::DocumentOnLoadCompletedInMainFrame() {
  // Mark the "medium bar" set
  medium_bar_met_ = true;

  if (renovations_enabled_) {
    // Run renovations. After renovations complete, a snapshot will be
    // triggered after a delay.
    client_->RunRenovations();
    // TODO(petewil): Someday we may add renovations that cause more image
    // loading - we can use the resource percentage signal.  We may need
    // to reset the page quality in case all the known images have loaded,
    // but the renovation makes more images available.
  } else {
    // If we are using the resource percentage signal, wait for resource
    // percentage to reach full instead of just snapshotting when we get the
    // event.  If not, start the snapshot now.
    if (!IsOfflinePagesResourceBasedSnapshotEnabled()) {
      // Post a delayed task to snapshot and then stop this controller.
      task_runner_->PostDelayedTask(
          FROM_HERE,
          base::Bind(&SnapshotController::MaybeStartSnapshotThenStop,
                     weak_ptr_factory_.GetWeakPtr()),
          base::TimeDelta::FromMilliseconds(
              delay_after_document_on_load_completed_ms_));
    }
  }
}

void SnapshotController::MaybeStartSnapshot() {
  UpdatePageQuality();
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

// A more advanced alternative for Right Moment Detection (RMD).
// Instead of just waiting for a timer after a load event, use signals
// to check the load progress, and snapshot when the page looks mostly complete.
void SnapshotController::UpdateLoadingResourceProgress(int images_requested,
                                                       int images_completed,
                                                       int css_requested,
                                                       int css_completed) {
  // Ensure we have met at least the low bar.
  if (!low_bar_met_ && !high_bar_met_) {
    return;
  }

  // We want all CSS to have arrived.
  if (css_requested > css_completed)
    return;

  // We want a high percentage of images.
  if (images_requested > 0 &&
      (kSnapshotTriggerFraction >
       (float)images_completed / (float)images_requested))
    return;

  // Set page quality to the highest value if we pass the tests.
  high_bar_met_ = true;

  // If we are waiting for renovations, we aren't done yet.
  if (renovations_enabled_ && !renovations_completed_)
    return;

  // If all the conditions are met, take a snapshot.  Page quality is now as
  // good as we think it will get.  Post a delayed task to snapshot and then
  // stop this controller.
  task_runner_->PostTask(
      FROM_HERE, base::Bind(&SnapshotController::MaybeStartSnapshotThenStop,
                            weak_ptr_factory_.GetWeakPtr()));
}

// Update the current page quality based on events we have seen.
void SnapshotController::UpdatePageQuality() {
  if (low_bar_met_)
    current_page_quality_ = PageQuality::FAIR_AND_IMPROVING;
  if (medium_bar_met_)
    current_page_quality_ = PageQuality::GOOD;
  if (high_bar_met_)
    current_page_quality_ = PageQuality::HIGH;
}

}  // namespace offline_pages
