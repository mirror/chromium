// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/common/mojo_video_frame_provider.h"

#include "media/mojo/common/mojo_shared_buffer_video_frame.h"

namespace media {

MojoVideoFrameProvider::MojoVideoFrameProvider() {}

MojoVideoFrameProvider::~MojoVideoFrameProvider() {}

scoped_refptr<VideoFrame> MojoVideoFrameProvider::CreateZeroInitializedFrame(
    VideoPixelFormat format,
    const gfx::Size& coded_size,
    const gfx::Rect& visible_rect,
    const gfx::Size& natural_size,
    base::TimeDelta timestamp) {
  // TODO(j.isorce): handle above sizes and implement
  // MojoSharedBufferVideoFrame::CreateZeroInitializedFrame.
  return MojoSharedBufferVideoFrame::CreateYUV(format, natural_size, timestamp);
}

scoped_refptr<VideoFrame> MojoVideoFrameProvider::WrapVideoFrame(
    const scoped_refptr<VideoFrame>& frame) {
  DCHECK(frame->storage_type() == VideoFrame::STORAGE_MOJO_SHARED_BUFFER);

  return MojoSharedBufferVideoFrame::WrapMojoSharedBufferVideoFrame(
      static_cast<media::MojoSharedBufferVideoFrame*>(frame.get()));
}

}  // namespace media
