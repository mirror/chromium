// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_VIDEO_DEFAULT_VIDEO_FRAME_PROVIDER_H_
#define MEDIA_VIDEO_DEFAULT_VIDEO_FRAME_PROVIDER_H_

#include "media/base/video_frame_provider.h"

namespace media {

class MEDIA_EXPORT DefaultVideoFrameProvider final : public VideoFrameProvider {
 public:
  DefaultVideoFrameProvider();
  ~DefaultVideoFrameProvider() override;

  std::unique_ptr<VideoFrame::Buffer> CreateBuffer(
      size_t data_size) const override;

  scoped_refptr<VideoFrame> CreateZeroInitializedFrame(
      VideoPixelFormat format,
      const gfx::Size& coded_size,
      const gfx::Rect& visible_rect,
      const gfx::Size& natural_size,
      base::TimeDelta timestamp) override;

  scoped_refptr<VideoFrame> WrapVideoFrame(
      const scoped_refptr<VideoFrame>& frame) override;

  scoped_refptr<VideoFrame> WrapExternalYuvBuffer(
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
      base::TimeDelta timestamp) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DefaultVideoFrameProvider);
};

}  // namespace media

#endif  // MEDIA_VIDEO_DEFAULT_VIDEO_FRAME_PROVIDER_H_
