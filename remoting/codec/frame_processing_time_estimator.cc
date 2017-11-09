// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/codec/frame_processing_time_estimator.h"

#include <algorithm>

#include "base/logging.h"
#include "remoting/base/constants.h"

namespace remoting {

namespace {

// We tracks the frame information in last 6 seconds.
static constexpr int kWindowSizeInSeconds = 6;

// A key-frame is assumed to be generated roughly every 3 seconds, though the
// accurate frequency is dependent on host/client software versions, the encoder
// being used, and the quality of the network.
static constexpr int kKeyFrameWindowSize = kWindowSizeInSeconds / 3;

// The count of delta frames we are tracking.
static constexpr int kDeltaFrameWindowSize =
    kTargetFrameRate * kWindowSizeInSeconds - kKeyFrameWindowSize;

base::TimeDelta CalculateEstimatedTransitTime(int size, int kbps) {
  return base::TimeDelta::FromMicroseconds(size * 1000 * 8 / kbps);
}

// Uses the |time| to estimate the frame rate, and round the result in ceiling.
// May return values over |kTargetFrameRate|.
int CalculateEstimatedFrameRate(base::TimeDelta time) {
  if (time.is_zero()) {
    return kTargetFrameRate;
  } else {
    int64_t us = time.InMicroseconds();
    return (base::Time::kMicrosecondsPerSecond + us - 1) / us;
  }
}

}  // namespace

FrameProcessingTimeEstimator::FrameProcessingTimeEstimator()
    : delta_frame_processing_us_(kDeltaFrameWindowSize),
      delta_frame_size_(kDeltaFrameWindowSize),
      key_frame_processing_us_(kKeyFrameWindowSize),
      key_frame_size_(kKeyFrameWindowSize),
      frame_finish_times_us_(kDeltaFrameWindowSize + kKeyFrameWindowSize),
      bandwidth_kbps_(kDeltaFrameWindowSize + kKeyFrameWindowSize) {}

FrameProcessingTimeEstimator::~FrameProcessingTimeEstimator() = default;

void FrameProcessingTimeEstimator::StartFrame() {
  start_time_ = Now();
}

void FrameProcessingTimeEstimator::FinishFrame(
    const WebrtcVideoEncoder::EncodedFrame& frame) {
  DCHECK(!start_time_.is_null());
  base::TimeTicks now = Now();
  frame_finish_times_us_.Record(now.since_origin().InMicroseconds());
  if (frame.key_frame) {
    key_frame_processing_us_.Record((now - start_time_).InMicroseconds());
    key_frame_size_.Record(frame.data.length());
    key_frame_count_++;
  } else {
    delta_frame_processing_us_.Record((now - start_time_).InMicroseconds());
    delta_frame_size_.Record(frame.data.length());
    delta_frame_count_++;
  }
  start_time_ = base::TimeTicks();
}

void FrameProcessingTimeEstimator::SetBandwidthKbps(int bandwidth_kbps) {
  if (bandwidth_kbps >= 0) {
    bandwidth_kbps_.Record(bandwidth_kbps);
  }
}

base::TimeDelta FrameProcessingTimeEstimator::EstimatedProcessingTime(
    bool key_frame) const {
  // Avoid returning 0 if there are no records for delta-frames.
  if ((key_frame && !key_frame_processing_us_.IsEmpty()) ||
      delta_frame_processing_us_.IsEmpty()) {
    return base::TimeDelta::FromMicroseconds(
        key_frame_processing_us_.Average());
  }
  return base::TimeDelta::FromMicroseconds(
      delta_frame_processing_us_.Average());
}

base::TimeDelta FrameProcessingTimeEstimator::EstimatedTransitTime(
    bool key_frame) const {
  if (bandwidth_kbps_.IsEmpty()) {
    // To avoid unnecessary complexity in WebrtcFrameSchedulerSimple, we return
    // a fairly large value (1 minute) here. So WebrtcFrameSchedulerSimple does
    // not need to handle the overflow issue caused by returning
    // TimeDelta::Max().
    return base::TimeDelta::FromMinutes(1);
  }
  // Avoid returning 0 if there are no records for delta-frames.
  if ((key_frame && !key_frame_size_.IsEmpty()) ||
      delta_frame_size_.IsEmpty()) {
    return CalculateEstimatedTransitTime(
        key_frame_size_.Average(), AverageBandwidthKbps());
  }
  return CalculateEstimatedTransitTime(
      delta_frame_size_.Average(), AverageBandwidthKbps());
}

int FrameProcessingTimeEstimator::AverageBandwidthKbps() const {
  return bandwidth_kbps_.Average();
}

int FrameProcessingTimeEstimator::EstimatedFrameSize() const {
  if (delta_frame_count_ + key_frame_count_ == 0) {
    return 0;
  }
  double key_frame_rate = key_frame_count_;
  key_frame_rate /= (delta_frame_count_ + key_frame_count_);
  return key_frame_rate * key_frame_size_.Average() +
         (1 - key_frame_rate) * delta_frame_size_.Average();
}

base::TimeDelta FrameProcessingTimeEstimator::EstimatedProcessingTime() const {
  if (delta_frame_count_ + key_frame_count_ == 0) {
    return base::TimeDelta();
  }
  double key_frame_rate = key_frame_count_;
  key_frame_rate /= (delta_frame_count_ + key_frame_count_);
  return base::TimeDelta::FromMicroseconds(
      key_frame_rate * key_frame_processing_us_.Average() +
      (1 - key_frame_rate) * delta_frame_processing_us_.Average());
}

base::TimeDelta FrameProcessingTimeEstimator::EstimatedTransitTime() const {
  if (bandwidth_kbps_.IsEmpty()) {
    return base::TimeDelta::FromMinutes(1);
  }
  return CalculateEstimatedTransitTime(
      EstimatedFrameSize(), AverageBandwidthKbps());
}

base::TimeDelta FrameProcessingTimeEstimator::
RecentAverageFrameInterval() const {
  if (frame_finish_times_us_.size() < 2) {
    return base::TimeDelta();
  }

  return base::TimeDelta::FromMicroseconds(
      (frame_finish_times_us_.last_sample() -
       frame_finish_times_us_.first_sample()) /
      (frame_finish_times_us_.size() - 1));
}

int FrameProcessingTimeEstimator::RecentFrameRate() const {
  return std::min(kTargetFrameRate,
                  CalculateEstimatedFrameRate(RecentAverageFrameInterval()));
}

int FrameProcessingTimeEstimator::PredictedFrameRate() const {
  return std::min({
      kTargetFrameRate,
      CalculateEstimatedFrameRate(EstimatedProcessingTime()),
      CalculateEstimatedFrameRate(EstimatedTransitTime())
  });
}

int FrameProcessingTimeEstimator::EstimatedFrameRate() const {
  return std::min(RecentFrameRate(), PredictedFrameRate());
}

base::TimeTicks FrameProcessingTimeEstimator::Now() const {
  return base::TimeTicks::Now();
}

}  // namespace remoting
