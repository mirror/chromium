// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display_embedder/display_output_surface_ozone.h"

#include "gpu/command_buffer/client/gles2_interface.h"
#include "ui/display/types/display_snapshot.h"

namespace viz {

DisplayOutputSurfaceOzone::DisplayOutputSurfaceOzone(
    scoped_refptr<InProcessContextProvider> context_provider,
    gfx::AcceleratedWidget widget,
    SyntheticBeginFrameSource* synthetic_begin_frame_source,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager)
    : SurfacelessDisplayOutputSurface(
          context_provider,
          widget,
          synthetic_begin_frame_source,
          gpu_memory_buffer_manager,
          GL_TEXTURE_2D,
          GL_RGB,
          display::DisplaySnapshot::PrimaryFormat()) {
  capabilities_.flipped_output_surface = true;
}

DisplayOutputSurfaceOzone::~DisplayOutputSurfaceOzone() {}

}  // namespace viz
