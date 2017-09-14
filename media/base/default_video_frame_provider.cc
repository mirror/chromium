// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/default_video_frame_provider.h"

namespace media {

DefaultVideoFrameProvider::DefaultVideoFrameProvider() {}

DefaultVideoFrameProvider::~DefaultVideoFrameProvider() {}

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

}  // namespace media
