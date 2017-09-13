// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/default_video_frame_provider_factory.h"

#include "base/memory/ptr_util.h"
#include "media/base/default_video_frame_provider.h"

namespace media {

DefaultVideoFrameProviderFactory::DefaultVideoFrameProviderFactory() {}

DefaultVideoFrameProviderFactory::~DefaultVideoFrameProviderFactory() {}

std::unique_ptr<VideoFrameProvider>
DefaultVideoFrameProviderFactory::CreateVideoFrameProvider() {
  return base::MakeUnique<DefaultVideoFrameProvider>();
}

}  // namespace media
