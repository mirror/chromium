// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_SCOPED_LOCAL_RESOURCE_H_
#define CC_RESOURCES_SCOPED_LOCAL_RESOURCE_H_

#include <memory>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "cc/cc_export.h"
#include "cc/resources/texture_id_allocator.h"
#include "components/viz/common/gpu/context_provider.h"
#include "components/viz/common/resources/resource_format.h"
#include "components/viz/common/resources/resource_texture_hint.h"
#include "components/viz/common/resources/resource_texture_settings.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "third_party/khronos/GLES2/gl2ext.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/size.h"

namespace cc {

class CC_EXPORT ScopedLocalResource {
 public:
  explicit ScopedLocalResource(
      viz::ContextProvider* compositor_context_provider,
      TextureIdAllocator* texture_id_allocator,
      const viz::ResourceTextureSettings* settings);
  ~ScopedLocalResource();

  void Allocate(const gfx::Size& size,
                viz::ResourceTextureHint hint,
                viz::ResourceFormat format,
                const gfx::ColorSpace& color_space);
  void Free();
  GLenum BindForSampling(GLenum unit, GLenum filter);

  // Returns null if we do not have a viz::ContextProvider.
  gpu::gles2::GLES2Interface* ContextGL() const;
  GLuint id() const { return gl_id_; }
  GLenum target() const { return target_; }
  const gfx::Size& size() const { return size_; }
  viz::ResourceFormat format() const { return format_; }
  const gfx::ColorSpace& color_space() const { return color_space_; }

  viz::ResourceTextureHint hint() const { return hint_; }
  void set_generate_mipmap() {
    generate_mipmap_ = true;
    mipmap_state_ = GENERATE;
  }

 private:
  enum MipmapState { INVALID, GENERATE, VALID };
  void CreateTexture();
  void DeleteResource();
  void AllocateWithGpuMemoryBuffer(const gfx::Size& size,
                                   viz::ResourceFormat format,
                                   gfx::BufferUsage usage,
                                   const gfx::ColorSpace& color_space);
  GLuint gl_id_ = 0;
  gfx::Size size_;
  GLenum target_ = GL_TEXTURE_2D;
  // TODO(skyostil): Use a separate sampler object for filter state.
  GLenum original_filter_ = GL_LINEAR;
  GLenum filter_ = GL_LINEAR;
  GLenum min_filter_ = GL_LINEAR;
  GLuint image_id_ = 0;
  viz::ResourceTextureHint hint_ = viz::ResourceTextureHint::kDefault;
  // Resource format is the format as seen from the compositor and might not
  // correspond to buffer_format (e.g: A resouce that was created from a YUV
  // buffer could be seen as RGB from the compositor/GL.)
  viz::ResourceFormat format_ = viz::ResourceFormat::RGBA_8888;
  gfx::ColorSpace color_space_;
  MipmapState mipmap_state_ = INVALID;
  viz::ContextProvider* compositor_context_provider_;
  TextureIdAllocator* texture_id_allocator_;
  const viz::ResourceTextureSettings* const settings_;

  bool is_overlay_ = false;
  bool allocated_ = false;
  bool generate_mipmap_ = false;

  DISALLOW_COPY_AND_ASSIGN(ScopedLocalResource);
};

}  // namespace cc

#endif  // CC_RESOURCES_SCOPED_LOCAL_RESOURCE_H_
