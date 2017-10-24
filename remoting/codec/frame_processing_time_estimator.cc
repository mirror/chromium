// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/codec/frame_processing_time_estimator.h"

#include "base/logging.h"
#include "remoting/base/constants.h"

namespace remoting {

namespace {

// We tracks the frame information in last 6 seconds.
static constexpr int kWindowSizeInSeconds = 6;

// The client requires a key frame once per 3 seconds.
static constexpr int kKeyFrameWindowSize = kWindowSizeInSeconds / 3;

// The count of delta frames we are tracking.
static constexpr int kDeltaFrameWindowSize =
    kTargetFrameRate * kWindowSizeInSeconds - kKeyFrameWindowSize;

base::TimeDelta EstimateProcessingTime(const RunningSamples& processing_us,
                                       const RunningSamples& size,
                                       const RunningSamples& bandwidth_kbps) {
  base::TimeDelta transite_time = base::TimeDelta::FromMilliseconds(
      size.Average() * 8 / bandwidth_kbps.Average());
  base::TimeDelta processing_time =
      base::TimeDelta::FromMicroseconds(processing_us.Average());
  return transite_time + processing_time;
}

}  // namespace

FrameProcessingTimeEstimator::FrameProcessingTimeEstimator()
    : delta_frame_processing_us_(kDeltaFrameWindowSize),
      delta_frame_size_(kDeltaFrameWindowSize),
      key_frame_processing_us_(kKeyFrameWindowSize),
      key_frame_size_(kKeyFrameWindowSize),
      bandwidth_kbps_(kDeltaFrameWindowSize + kKeyFrameWindowSize) {}

FrameProcessingTimeEstimator::~FrameProcessingTimeEstimator() = default;

void FrameProcessingTimeEstimator::StartFrame() {
  DCHECK(start_time_.is_null());
  start_time_ = Now();
}

void FrameProcessingTimeEstimator::FinishFrame(
    const WebrtcVideoEncoder::EncodedFrame& frame) {
  DCHECK(!start_time_.is_null());
  base::TimeTicks now = Now();
  if (frame.key_frame) {
    key_frame_processing_us_.Record((now - start_time_).InMicroseconds());
    key_frame_size_.Record(frame.data.length());
  } else {
    delta_frame_processing_us_.Record((now - start_time_).InMicroseconds());
    delta_frame_size_.Record(frame.data.length());
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
  if (bandwidth_kbps_.Average() == 0) {
    // To avoid unnecessary complexity in WebrtcFrameSchedulerSimple, we return
    // a fairly large value (1 minute) here. So WebrtcFrameSchedulerSimple does
    // not need to handle the overflow issue caused by returning
    // TimeDelta::Max().
    return base::TimeDelta::FromMinutes(1);
  }
  if (key_frame) {
    return EstimateProcessingTime(
        key_frame_processing_us_, key_frame_size_, bandwidth_kbps_);
  } else {
    return EstimateProcessingTime(
        delta_frame_processing_us_, delta_frame_size_, bandwidth_kbps_);
  }
}

base::TimeTicks FrameProcessingTimeEstimator::Now() const {
  return base::TimeTicks::Now();
}

}  // namespace remoting
