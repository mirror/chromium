// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/watch_time_sub_reporter.h"

#include "media/base/watch_time_keys.h"
#include "media/blink/watch_time_reporter.h"

namespace media {

// TODO: have a shared location.
constexpr base::TimeDelta kMinimumElapsedTime = base::TimeDelta::FromSeconds(7);

WatchTimeSubReporter::WatchTimeSubReporter(WatchTimeReporter* watch_time_reporter)
  : watch_time_reporter_(watch_time_reporter) {
}

WatchTimeSubReporter::~WatchTimeSubReporter() = default;

void WatchTimeSubReporter::OnStartReportingTimer(base::TimeDelta start_timestamp) {
  last_timestamp_ = end_timestamp_ = kNoTimestamp;
  start_timestamp = kNoTimestamp;
}

void WatchTimeSubReporter::TriggerUpdate() {
  end_timestamp_ = watch_time_reporter_->get_media_time_cb_.Run();
  watch_time_reporter_->reporting_timer_.Start(
      FROM_HERE, watch_time_reporter_->reporting_interval_,
      watch_time_reporter_, &WatchTimeReporter::UpdateWatchTime);
}

bool WatchTimeSubReporter::IsBackground() const {
  return watch_time_reporter_->is_background_;
}

bool WatchTimeSubReporter::HasVideo() const {
  return watch_time_reporter_->has_video_;
}

bool WatchTimeSubReporter::TimerIsRunning() const {
  return watch_time_reporter_->reporting_timer_.IsRunning();
}

bool WatchTimeSubReporter::IsChangePending() const {
  return end_timestamp_ != kNoTimestamp;
}

void WatchTimeSubReporter::OnUpdateWatchTime(base::TimeDelta current_timestamp,
                                             MediaLogEvent* log_event) {
  if (last_timestamp_ == current_timestamp)
    return;
  last_timestamp_ = IsChangePending() ? end_timestamp_ : current_timestamp;

  const base::TimeDelta elapsed_time = last_timestamp_ - start_timestamp_;
  if (elapsed_time >= kMinimumElapsedTime) {
    log_event->params.SetDoubleWithoutPathExpansion(GetRecordingKey(),
                                                    elapsed_time.InSecondsF());
  }

  if (IsChangePending()) {
    log_event->params.SetBoolean(GetFinalizeKey(), true);

    UpdateValueAfterCommit();
    start_timestamp_ = end_timestamp_;
    end_timestamp_ = kNoTimestamp;
  }
}

}  // namespace media
