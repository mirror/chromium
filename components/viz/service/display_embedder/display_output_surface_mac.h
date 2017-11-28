// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_DISPLAY_OUTPUT_SURFACE_MAC_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_DISPLAY_OUTPUT_SURFACE_MAC_H_

#include "components/viz/service/display_embedder/surfaceless_display_output_surface.h"

namespace viz {

class DisplayOutputSurfaceMac : public SurfacelessDisplayOutputSurface {
 public:
  DisplayOutputSurfaceMac(
      scoped_refptr<InProcessContextProvider> context_provider,
      gfx::AcceleratedWidget widget,
      SyntheticBeginFrameSource* synthetic_begin_frame_source,
      gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager);

  ~DisplayOutputSurfaceMac() override;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_DISPLAY_OUTPUT_SURFACE_MAC_H_
