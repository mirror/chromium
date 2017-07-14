// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/bitrate_estimator.h"

#include "base/time/default_tick_clock.h"

namespace media {

namespace {
constexpr int kBitsPerByte = 8;
constexpr int kMicrosecondsPerSecond = 1000000;
// The minimum estimation duration to get a valid estimated bitrate.
constexpr base::TimeDelta kMinEstimationDuration =
    base::TimeDelta::FromSeconds(1);
}  // namespace

BitrateEstimator::BitrateEstimator()
    : clock_(new base::DefaultTickClock()), start_time_(clock_->NowTicks()) {}

BitrateEstimator::~BitrateEstimator() {}

void BitrateEstimator::EnqueueOneFrame(int frame_bytes) {
  DCHECK(frame_bytes);
  base::AutoLock auto_lock(lock_);

  data_in_bytes_ += frame_bytes;
}

double BitrateEstimator::GetBitrate() {
  base::AutoLock auto_lock(lock_);
  if (!data_in_bytes_)
    return 0;
  const base::TimeDelta duration = clock_->NowTicks() - start_time_;
  const int duration_us = duration.InMicroseconds();
  return duration < kMinEstimationDuration
             ? 0.0
             : static_cast<double>(data_in_bytes_) * kBitsPerByte *
                   kMicrosecondsPerSecond / duration_us;
}

void BitrateEstimator::Reset() {
  base::AutoLock auto_lock(lock_);
  data_in_bytes_ = 0;
  start_time_ = clock_->NowTicks();
}

}  // namespace media
