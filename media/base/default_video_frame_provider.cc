// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/default_video_frame_provider.h"

#include "base/memory/ptr_util.h"

namespace media {

DefaultVideoFrameProvider::DefaultVideoFrameProvider() {}

DefaultVideoFrameProvider::~DefaultVideoFrameProvider() {}

std::unique_ptr<VideoFrame::Buffer> DefaultVideoFrameProvider::CreateBuffer(
    size_t data_size) const {
  return base::MakeUnique<VideoFrame::BasicBuffer>(data_size);
}

scoped_refptr<VideoFrame> DefaultVideoFrameProvider::CreateZeroInitializedFrame(
    VideoPixelFormat format,
    const gfx::Size& coded_size,
    const gfx::Rect& visible_rect,
    const gfx::Size& natural_size,
    base::TimeDelta timestamp) {
  return VideoFrame::CreateZeroInitializedFrame(
      format, coded_size, visible_rect, natural_size, timestamp);
}

scoped_refptr<VideoFrame> DefaultVideoFrameProvider::WrapVideoFrame(
    const scoped_refptr<VideoFrame>& frame) {
  return VideoFrame::WrapVideoFrame(
      frame, frame->format(), frame->visible_rect(), frame->natural_size());
}

scoped_refptr<VideoFrame> DefaultVideoFrameProvider::WrapExternalYuvBuffer(
    VideoFrame::Buffer* buffer,
    VideoPixelFormat format,
    const gfx::Size& coded_size,
    const gfx::Rect& visible_rect,
    const gfx::Size& natural_size,
    int32_t y_stride,
    int32_t u_stride,
    int32_t v_stride,
    uint8_t* y_data,
    uint8_t* u_data,
    uint8_t* v_data,
    base::TimeDelta timestamp) const {
  return VideoFrame::WrapExternalYuvData(
      format, coded_size, visible_rect, natural_size, y_stride, u_stride,
      v_stride, y_data, u_data, v_data, timestamp);
}

}  // namespace media
