// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/bitrate_estimator.h"

namespace media {

namespace {
constexpr int kBitsPerByte = 8;
constexpr int kMicrosecondsPerSecond = 1000000;
}  // namespace

BitrateEstimator::BitrateEstimator(base::TimeDelta duration,
                                   const ResultCB& callback)
    : duration_(duration), callback_(callback) {
  DCHECK(duration_ > base::TimeDelta());
  DCHECK(!callback_.is_null());
}

BitrateEstimator::~BitrateEstimator() {}

void BitrateEstimator::EnqueueOneFrame(int frame_bytes,
                                       base::TimeDelta timestamp,
                                       bool is_key_frame) {
  DCHECK(timestamp > base::TimeDelta());
  DCHECK(frame_bytes);

  base::TimeDelta duration;
  const int64_t total_size = EnqueueFrameAndUpdate_Locked(
      frame_bytes, timestamp, is_key_frame, &duration);
  if (total_size && duration >= duration_) {
    // This BitrateEstimator may be destroyed after this call.
    CalculateAndReportBitrate(duration, total_size);
  }
}

int64_t BitrateEstimator::EnqueueFrameAndUpdate_Locked(
    int frame_bytes,
    base::TimeDelta timestamp,
    bool is_key_frame,
    base::TimeDelta* duration) {
  DCHECK(duration);
  base::AutoLock auto_lock(lock_);

  *duration = base::TimeDelta();
  // The first frame enqueued.
  if (!data_in_bytes_) {
    // Wait for the first key frame enqueued to start the estimation.
    if (!is_key_frame)
      return 0;
    min_timestamp_ = timestamp;
    max_timestamp_ = timestamp;
    data_in_bytes_ += frame_bytes;
    return 0;
  }

  if (timestamp < min_timestamp_) {
    min_timestamp_ = timestamp;
    data_in_bytes_ += frame_bytes;
    return 0;
  }

  if (timestamp < max_timestamp_) {
    data_in_bytes_ += frame_bytes;
    return 0;
  }

  const base::TimeDelta current_duration = max_timestamp_ - min_timestamp_;
  const int64_t data_in_bytes = data_in_bytes_;
  if (current_duration < duration_) {
    max_timestamp_ = timestamp;
    data_in_bytes_ += frame_bytes;
    return 0;
  }

  // Enqueue this frame as the first frame for next estimate.
  min_timestamp_ = max_timestamp_ = timestamp;
  data_in_bytes_ = frame_bytes;

  *duration = current_duration;
  return data_in_bytes;
}

void BitrateEstimator::CalculateAndReportBitrate(base::TimeDelta duration,
                                                 int64_t data_in_bytes) {
  const Status status = duration < duration_ ? Status::kAborted : Status::kOk;
  const int duration_us = duration.InMicroseconds();
  const double bps = duration_us
                         ? static_cast<double>(data_in_bytes) * kBitsPerByte *
                               kMicrosecondsPerSecond / duration_us
                         : 0.0;
  callback_.Run(status, static_cast<int>(bps));
}

int64_t BitrateEstimator::GetStats_Locked(base::TimeDelta* duration) {
  DCHECK(duration);
  base::AutoLock auto_lock(lock_);
  *duration = max_timestamp_ - min_timestamp_;
  return data_in_bytes_;
}

void BitrateEstimator::ReportResult() {
  base::TimeDelta duration;
  const int64_t total_size = GetStats_Locked(&duration);

  // This BitrateEstimator may be destroyed after this call.
  CalculateAndReportBitrate(duration, total_size);
}

}  // namespace media
