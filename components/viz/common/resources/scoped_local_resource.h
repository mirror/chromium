// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCES_SCOPED_LOCAL_RESOURCE_H_
#define COMPONENTS_VIZ_COMMON_RESOURCES_SCOPED_LOCAL_RESOURCE_H_

#include "components/viz/common/resources/resource_format.h"
#include "components/viz/common/resources/resource_texture_hint.h"
#include "components/viz/common/viz_common_export.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/size.h"

namespace viz {

// ScopedLocalResource is resource used inside the same GL context and will not
// being sent into another process. So no need to create fence and mailbox for
// these resources.
class VIZ_COMMON_EXPORT ScopedLocalResource {
 public:
  explicit ScopedLocalResource(gpu::gles2::GLES2Interface* gl,
                               const bool use_texture_usage_hint,
                               const bool use_texture_storage);
  ~ScopedLocalResource();

  void Allocate(const gfx::Size& size, const gfx::ColorSpace& color_space);
  void Free();
  void BindForSampling(GLenum unit);

  GLuint id() const { return gl_id_; }
  GLenum target() const { return GL_TEXTURE_2D; }
  const gfx::Size& size() const { return size_; }
  ResourceFormat format() const { return ResourceFormat::RGBA_8888; }
  const gfx::ColorSpace& color_space() const { return color_space_; }
  ResourceTextureHint hint() const { return ResourceTextureHint::kFramebuffer; }

 private:
  void CreateTexture();

  gpu::gles2::GLES2Interface* gl_;
  // Texture id for texture-backed resources.
  GLuint gl_id_ = 0;
  // Size of the resource in pixels.
  gfx::Size size_;
  gfx::ColorSpace color_space_;
  // When false, the resource backing hasn't been allocated yet.
  bool allocated_ = false;
  const bool use_texture_usage_hint_;
  const bool use_texture_storage_;

  DISALLOW_COPY_AND_ASSIGN(ScopedLocalResource);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCES_SCOPED_LOCAL_RESOURCE_H_