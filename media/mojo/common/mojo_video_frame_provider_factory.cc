// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/common/mojo_video_frame_provider_factory.h"

#include "base/memory/ptr_util.h"
#include "media/mojo/common/mojo_video_frame_provider.h"

namespace media {

MojoVideoFrameProviderFactory::MojoVideoFrameProviderFactory() {}

MojoVideoFrameProviderFactory::~MojoVideoFrameProviderFactory() {}

std::unique_ptr<VideoFrameProvider>
MojoVideoFrameProviderFactory::CreateVideoFrameProvider() {
  return base::MakeUnique<MojoVideoFrameProvider>();
}

}  // namespace media
