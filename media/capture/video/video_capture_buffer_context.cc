// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/video_capture_buffer_context.h"

namespace media {

VideoCaptureBufferContext::VideoCaptureBufferContext(
    int buffer_context_id,
    int buffer_id,
    VideoFrameConsumerFeedbackObserver* consumer_feedback_observer,
    mojo::ScopedSharedBufferHandle handle)
    : buffer_context_id_(buffer_context_id),
      buffer_id_(buffer_id),
      is_retired_(false),
      frame_feedback_id_(0),
      consumer_feedback_observer_(consumer_feedback_observer),
      buffer_handle_(std::move(handle)),
      max_consumer_utilization_(
          VideoFrameConsumerFeedbackObserver::kNoUtilizationRecorded),
      consumer_hold_count_(0) {}

VideoCaptureBufferContext::~VideoCaptureBufferContext() = default;

VideoCaptureBufferContext::VideoCaptureBufferContext(
    VideoCaptureBufferContext&& other) = default;

VideoCaptureBufferContext& VideoCaptureBufferContext::operator=(
    VideoCaptureBufferContext&& other) = default;

void VideoCaptureBufferContext::RecordConsumerUtilization(double utilization) {
  if (std::isfinite(utilization) && utilization >= 0.0) {
    max_consumer_utilization_ =
        std::max(max_consumer_utilization_, utilization);
  }
}

void VideoCaptureBufferContext::IncreaseConsumerCount() {
  consumer_hold_count_++;
}

void VideoCaptureBufferContext::DecreaseConsumerCount() {
  consumer_hold_count_--;
  if (consumer_hold_count_ == 0) {
    if (consumer_feedback_observer_ != nullptr &&
        max_consumer_utilization_ !=
            VideoFrameConsumerFeedbackObserver::kNoUtilizationRecorded) {
      consumer_feedback_observer_->OnUtilizationReport(
          frame_feedback_id_, max_consumer_utilization_);
    }
    buffer_read_permission_.reset();
    max_consumer_utilization_ =
        VideoFrameConsumerFeedbackObserver::kNoUtilizationRecorded;
  }
}

mojo::ScopedSharedBufferHandle VideoCaptureBufferContext::CloneHandle() {
  // Special behavior here: If the handle was already read-only, the Clone()
  // call here will maintain that read-only permission. If it was read-write,
  // the cloned handle will have read-write permission.
  //
  // TODO(crbug.com/797470): We should be able to demote read-write to read-only
  // permissions when Clone()'ing handles. Currently, this causes a crash.
  return buffer_handle_->Clone(
      mojo::SharedBufferHandle::AccessMode::READ_WRITE);
}

}  // namespace media
