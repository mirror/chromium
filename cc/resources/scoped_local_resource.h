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
#include "components/viz/common/gpu/context_provider.h"
#include "components/viz/common/resources/resource_format.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "third_party/khronos/GLES2/gl2ext.h"
// TODO(xing.xu): remove below
#include "cc/resources/resource_provider.h"
#include "cc/resources/texture_id_allocator.h"

/*
#if DCHECK_IS_ON()
#include "base/threading/platform_thread.h"
#endif
*/
namespace cc {

class CC_EXPORT ScopedLocalResource {
 public:
  explicit ScopedLocalResource(
      viz::ContextProvider* compositor_context_provider,
      TextureIdAllocator* texture_id_allocator,
      const ResourceProvider::Settings* settings);
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
  // uint8_t* pixels = nullptr;
  gfx::Size size_;
  // Origin origin = INTERNAL;
  GLenum target_ = GL_TEXTURE_2D;
  // TODO(skyostil): Use a separate sampler object for filter state.
  GLenum original_filter_ = GL_LINEAR;
  GLenum filter_ = GL_LINEAR;
  GLenum min_filter_ = GL_LINEAR;
  GLuint image_id_ = 0;
  viz::ResourceTextureHint hint_ = viz::ResourceTextureHint::kDefault;
  // ResourceProvider::ResourceType type_ =
  // ResourceProvider::RESOURCE_TYPE_BITMAP;
  // GpuMemoryBuffer resource allocation needs to know how the resource will
  // be used.
  // gfx::BufferUsage usage_ = gfx::BufferUsage::GPU_READ_CPU_READ_WRITE;
  // This is the the actual format of the underlaying GpuMemoryBuffer, if any,
  // and might not correspond to viz::ResourceFormat. This format is needed to
  // scanout the buffer as HW overlay.
  // gfx::BufferFormat buffer_format_ = gfx::BufferFormat::RGBA_8888;
  // Resource format is the format as seen from the compositor and might not
  // correspond to buffer_format (e.g: A resouce that was created from a YUV
  // buffer could be seen as RGB from the compositor/GL.)
  viz::ResourceFormat format_ = viz::ResourceFormat::RGBA_8888;
  // viz::SharedBitmapId shared_bitmap_id;
  // viz::SharedBitmap* shared_bitmap = nullptr;
  // std::unique_ptr<viz::SharedBitmap> owned_shared_bitmap;
  // std::unique_ptr<gfx::GpuMemoryBuffer> gpu_memory_buffer;
  gfx::ColorSpace color_space_;
  MipmapState mipmap_state_ = INVALID;
  viz::ContextProvider* compositor_context_provider_;
  TextureIdAllocator* texture_id_allocator_;
  const ResourceProvider::Settings* const settings_;

  bool is_overlay_ = false;
  bool allocated_ = false;
  bool generate_mipmap_ = false;

  DISALLOW_COPY_AND_ASSIGN(ScopedLocalResource);
};

}  // namespace cc

#endif  // CC_RESOURCES_SCOPED_LOCAL_RESOURCE_H_
