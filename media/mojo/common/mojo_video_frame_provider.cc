// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/common/mojo_video_frame_provider.h"

#include "base/memory/ptr_util.h"
#include "media/mojo/common/mojo_shared_buffer_video_frame.h"

namespace media {

MojoVideoFrameProvider::MojoVideoFrameProvider() {}

MojoVideoFrameProvider::~MojoVideoFrameProvider() {}

std::unique_ptr<VideoFrame::Buffer> MojoVideoFrameProvider::CreateBuffer(
    size_t data_size) const {
  std::unique_ptr<MojoSharedBufferVideoFrame::MojoBuffer> buffer =
      base::MakeUnique<MojoSharedBufferVideoFrame::MojoBuffer>(data_size);

  if (!buffer->is_valid())
    return nullptr;

  return buffer;
}

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

scoped_refptr<VideoFrame> MojoVideoFrameProvider::WrapExternalYuvBuffer(
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
  mojo::ScopedSharedBufferHandle duplicated_handle =
      static_cast<MojoSharedBufferVideoFrame::MojoBuffer*>(buffer)
          ->Handle()
          .Clone();
  CHECK(duplicated_handle.is_valid());

  return MojoSharedBufferVideoFrame::Create(
      format, coded_size, visible_rect, natural_size,
      std::move(duplicated_handle), buffer->size(), y_data - buffer->data(),
      u_data - buffer->data(), v_data - buffer->data(), y_stride, u_stride,
      v_stride, timestamp);
}

}  // namespace media
