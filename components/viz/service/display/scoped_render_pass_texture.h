// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_SCOPED_RENDER_PASS_TEXTURE_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_SCOPED_RENDER_PASS_TEXTURE_H_

#include "base/macros.h"
#include "components/viz/common/resources/resource_format.h"
#include "components/viz/common/resources/resource_texture_hint.h"
#include "components/viz/service/viz_service_export.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "ui/gfx/geometry/size.h"

namespace viz {

// ScopedRenderPassTexture is resource used inside the same GL context and will
// not being sent into another process. So no need to create fence and mailbox
// for these resources.
class VIZ_SERVICE_EXPORT ScopedRenderPassTexture {
 public:
  explicit ScopedRenderPassTexture(gpu::gles2::GLES2Interface* gl,
                                   const bool use_texture_usage_hint,
                                   const bool use_texture_storage);
  ~ScopedRenderPassTexture();

  void Allocate(const gfx::Size& size);
  void Free();
  void BindForSampling();

  GLuint id() const { return gl_id_; }
  const gfx::Size& size() const { return size_; }

 private:
  void CreateTexture();

  gpu::gles2::GLES2Interface* gl_;
  // Texture id for texture-backed resources.
  GLuint gl_id_ = 0;
  // Size of the resource in pixels.
  gfx::Size size_;
  // When false, the resource backing hasn't been allocated yet.
  bool allocated_ = false;
  const bool use_texture_usage_hint_;
  const bool use_texture_storage_;

  DISALLOW_COPY_AND_ASSIGN(ScopedRenderPassTexture);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_SCOPED_RENDER_PASS_TEXTURE_H_