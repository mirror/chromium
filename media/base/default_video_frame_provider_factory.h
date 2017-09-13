// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_VIDEO_DEFAULT_VIDEO_FRAME_PROVIDER_FACTORY_H_
#define MEDIA_VIDEO_DEFAULT_VIDEO_FRAME_PROVIDER_FACTORY_H_

#include "media/base/video_frame_provider_factory.h"

namespace media {

class MEDIA_EXPORT DefaultVideoFrameProviderFactory final
    : public VideoFrameProviderFactory {
 public:
  DefaultVideoFrameProviderFactory();
  ~DefaultVideoFrameProviderFactory() override;

  std::unique_ptr<VideoFrameProvider> CreateVideoFrameProvider() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DefaultVideoFrameProviderFactory);
};

}  // namespace media

#endif  // MEDIA_VIDEO_DEFAULT_VIDEO_FRAME_PROVIDER_FACTORY_H_
