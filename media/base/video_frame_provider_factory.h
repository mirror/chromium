// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_VIDEO_FRAME_PROVIDER_FACTORY_H_
#define MEDIA_BASE_VIDEO_FRAME_PROVIDER_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "media/base/media_export.h"

namespace media {

class VideoFrameProvider;

class MEDIA_EXPORT VideoFrameProviderFactory {
 public:
  VideoFrameProviderFactory();
  virtual ~VideoFrameProviderFactory();

  virtual std::unique_ptr<VideoFrameProvider> CreateVideoFrameProvider() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(VideoFrameProviderFactory);
};

}  // namespace media

#endif  // MEDIA_BASE_VIDEO_FRAME_PROVIDER_FACTORY_H_
