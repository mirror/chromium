// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_GPU_TEXTURE_ALLOCATION_H_
#define COMPONENTS_VIZ_COMMON_GPU_TEXTURE_ALLOCATION_H_

#include "components/viz/common/resources/resource_format.h"
#include "components/viz/common/viz_common_export.h"
#include "third_party/khronos/GLES2/gl2.h"

#include <stdint.h>

namespace gfx {
class ColorSpace;
class Size;
}  // namespace gfx

namespace gpu {
struct Capabilities;
namespace gles2 {
class GLES2Interface;
}
}  // namespace gpu

namespace viz {

class VIZ_COMMON_EXPORT TextureAllocation {
 public:
  GLuint texture_id = 0;
  GLenum texture_target = 0;
  bool overlay_candidate = false;

  static TextureAllocation Allocate(gpu::gles2::GLES2Interface* gl,
                                    const gpu::Capabilities& caps,
                                    ResourceFormat format,
                                    const gfx::Size& size,
                                    const gfx::ColorSpace& color_space,
                                    bool use_gpu_memory_buffer_resources,
                                    bool for_framebuffer_attachment);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_GPU_TEXTURE_ALLOCATION_H_
