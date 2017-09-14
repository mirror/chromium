// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_VIDEO_FRAME_PROVIDER_H_
#define MEDIA_BASE_VIDEO_FRAME_PROVIDER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "media/base/media_export.h"
#include "media/base/video_frame.h"

namespace media {

class MEDIA_EXPORT VideoFrameProvider {
 public:
  VideoFrameProvider();
  virtual ~VideoFrameProvider();
  virtual scoped_refptr<VideoFrame> CreateZeroInitializedFrame(
      VideoPixelFormat format,
      const gfx::Size& coded_size,
      const gfx::Rect& visible_rect,
      const gfx::Size& natural_size,
      base::TimeDelta timestamp) = 0;

  virtual scoped_refptr<VideoFrame> WrapVideoFrame(
      const scoped_refptr<VideoFrame>& frame) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(VideoFrameProvider);
};

}  // namespace media

#endif  // MEDIA_BASE_VIDEO_FRAME_PROVIDER_H_
