// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/trees/render_frame_metadata.h"

namespace cc {

RenderFrameMetadata::RenderFrameMetadata() = default;

RenderFrameMetadata::RenderFrameMetadata(RenderFrameMetadata&& other) = default;

RenderFrameMetadata::~RenderFrameMetadata() {}

RenderFrameMetadata& RenderFrameMetadata::operator=(
    RenderFrameMetadata&& other) = default;

RenderFrameMetadata RenderFrameMetadata::Clone() const {
  RenderFrameMetadata metadata(*this);
  return metadata;
}

RenderFrameMetadata::RenderFrameMetadata(const RenderFrameMetadata& other) =
    default;

}  // namespace cc
