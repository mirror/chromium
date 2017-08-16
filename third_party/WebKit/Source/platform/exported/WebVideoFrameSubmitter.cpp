// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/platform/WebVideoFrameSubmitter.h"

#include "third_party/WebKit/Source/platform/graphics/VideoFrameSubmitter.h"
#include "third_party/WebKit/Source/platform/wtf/Functional.h"

namespace cc {
class VideoFrameProvider;
}  // namespace cc

namespace blink {

std::unique_ptr<WebVideoFrameSubmitter> WebVideoFrameSubmitter::Create(
    cc::VideoFrameProvider* provider,
    const viz::FrameSinkId& id) {
  return base::MakeUnique<VideoFrameSubmitter>(
      provider, WTF::Bind(&VideoFrameSubmitter::CreateCompositorFrameSink, id));
}

}  // namespace blink
