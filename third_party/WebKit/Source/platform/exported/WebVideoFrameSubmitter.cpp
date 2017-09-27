// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/platform/WebVideoFrameSubmitter.h"

#include <memory>
#include "third_party/WebKit/Source/platform/graphics/VideoFrameSubmitter.h"

namespace cc {
class VideoFrameProvider;
}  // namespace cc

namespace viz {
class ContextProvider;
}

namespace blink {

std::unique_ptr<WebVideoFrameSubmitter> WebVideoFrameSubmitter::Create(
    cc::VideoFrameProvider* provider,
    const media::ContextProviderCallback& context_provider_callback) {
  return base::MakeUnique<VideoFrameSubmitter>(provider,
                                               context_provider_callback);
}

}  // namespace blink
