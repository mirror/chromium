// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_QUADS_RENDER_FRAME_METADATA_H_
#define COMPONENTS_VIZ_COMMON_QUADS_RENDER_FRAME_METADATA_H_

#include "components/viz/common/viz_common_export.h"
#include "ui/gfx/geometry/vector2d_f.h"

namespace viz {

class VIZ_COMMON_EXPORT RenderFrameMetadata {
 public:
  RenderFrameMetadata();
  RenderFrameMetadata(RenderFrameMetadata&& other);
  ~RenderFrameMetadata();

  RenderFrameMetadata& operator=(RenderFrameMetadata&& other);

  RenderFrameMetadata Clone() const;

  // Scroll offset and scale of the root layer. This can be used for tasks
  // like positioning windowed plugins.
  gfx::Vector2dF root_scroll_offset;

 private:
  RenderFrameMetadata(const RenderFrameMetadata& other);
  RenderFrameMetadata operator=(const RenderFrameMetadata&) = delete;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_QUADS_RENDER_FRAME_METADATA_H_
