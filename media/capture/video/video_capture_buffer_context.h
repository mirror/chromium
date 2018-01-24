// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_VIDEO_CAPTURE_BUFFER_CONTEXT_H_
#define MEDIA_CAPTURE_VIDEO_VIDEO_CAPTURE_BUFFER_CONTEXT_H_

#include "media/capture/capture_export.h"
#include "media/capture/video/video_capture_device.h"

namespace media {

class CAPTURE_EXPORT VideoCaptureBufferContext {
 public:
  VideoCaptureBufferContext(
      int buffer_context_id,
      int buffer_id,
      VideoFrameConsumerFeedbackObserver* consumer_feedback_observer,
      mojo::ScopedSharedBufferHandle handle);
  ~VideoCaptureBufferContext();
  VideoCaptureBufferContext(VideoCaptureBufferContext&& other);
  VideoCaptureBufferContext& operator=(VideoCaptureBufferContext&& other);
  int buffer_context_id() const { return buffer_context_id_; }
  int buffer_id() const { return buffer_id_; }
  bool is_retired() const { return is_retired_; }
  void set_is_retired() { is_retired_ = true; }
  void set_frame_feedback_id(int id) { frame_feedback_id_ = id; }
  void set_consumer_feedback_observer(
      VideoFrameConsumerFeedbackObserver* consumer_feedback_observer) {
    consumer_feedback_observer_ = consumer_feedback_observer;
  }
  void set_read_permission(
      std::unique_ptr<
          VideoCaptureDevice::Client::Buffer::ScopedAccessPermission>
          buffer_read_permission) {
    buffer_read_permission_ = std::move(buffer_read_permission);
  }
  void RecordConsumerUtilization(double utilization);
  void IncreaseConsumerCount();
  void DecreaseConsumerCount();
  bool HasConsumers() const { return consumer_hold_count_ > 0; }
  mojo::ScopedSharedBufferHandle CloneHandle();

 private:
  int buffer_context_id_;
  int buffer_id_;
  bool is_retired_;
  int frame_feedback_id_;
  media::VideoFrameConsumerFeedbackObserver* consumer_feedback_observer_;
  mojo::ScopedSharedBufferHandle buffer_handle_;
  double max_consumer_utilization_;
  int consumer_hold_count_;
  std::unique_ptr<
      media::VideoCaptureDevice::Client::Buffer::ScopedAccessPermission>
      buffer_read_permission_;

  DISALLOW_COPY_AND_ASSIGN(VideoCaptureBufferContext);
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_VIDEO_CAPTURE_BUFFER_CONTEXT_H_
