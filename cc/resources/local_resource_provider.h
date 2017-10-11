// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_LOCAL_RESOURCE_PROVIDER_H_
#define CC_RESOURCES_LOCAL_RESOURCE_PROVIDER_H_

#include "build/build_config.h"
#include "cc/output/overlay_candidate.h"
#include "cc/resources/resource_provider.h"

namespace cc {

// This class is not thread-safe and can only be called from the thread it was
// created on.
class CC_EXPORT LocalResourceProvider : public ResourceProvider {
 public:
  LocalResourceProvider(viz::ContextProvider* compositor_context_provider,
                        gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
                        const viz::ResourceSettings& resource_settings);
  ~LocalResourceProvider() override;

  // The following scoped classes are part of the LocalResourceProvider API
  // and are needed to read the resource contents in the same gl context.
  class CC_EXPORT ScopedReadGL {
   public:
    ScopedReadGL(LocalResourceProvider* resource_provider,
                 viz::ResourceId resource_id);
    ~ScopedReadGL();

    GLuint texture_id() const { return resource_->gl_id; }
    GLenum target() const { return resource_->target; }
    const gfx::Size& size() const { return resource_->size; }
    const gfx::ColorSpace& color_space() const {
      return resource_->color_space;
    }

   private:
    const Resource* resource_;

    DISALLOW_COPY_AND_ASSIGN(ScopedReadGL);
  };

  class CC_EXPORT ScopedSamplerGL {
   public:
    ScopedSamplerGL(LocalResourceProvider* resource_provider,
                    viz::ResourceId resource_id,
                    GLenum filter);
    ScopedSamplerGL(LocalResourceProvider* resource_provider,
                    viz::ResourceId resource_id,
                    GLenum unit,
                    GLenum filter);
    ~ScopedSamplerGL();

    GLuint texture_id() const { return resource_lock_.texture_id(); }
    GLenum target() const { return target_; }
    const gfx::ColorSpace& color_space() const {
      return resource_lock_.color_space();
    }

   private:
    const ScopedReadGL resource_lock_;
    const GLenum unit_;
    const GLenum target_;

    DISALLOW_COPY_AND_ASSIGN(ScopedSamplerGL);
  };

  // The following scoped classes are part of the LocalResourceProvider API
  // and are needed to write the resource contents in the same gl context.
  class CC_EXPORT ScopedUseGL {
   public:
    ScopedUseGL(LocalResourceProvider* resource_provider,
                viz::ResourceId resource_id);
    ~ScopedUseGL();

    GLenum target() const { return resource_->target; }
    viz::ResourceFormat format() const { return resource_->format; }
    const gfx::Size& size() const { return resource_->size; }

    void set_generate_mipmap() { generate_mipmap_ = true; }

    // Returns texture id on compositor context, allocating if necessary.
    GLuint GetTexture() { return resource_->gl_id; }

   private:
    void AllocateGpuMemoryBuffer(gpu::gles2::GLES2Interface* gl,
                                 GLuint texture_id);

    void AllocateTexture(gpu::gles2::GLES2Interface* gl, GLuint texture_id);

    LocalResourceProvider* const resource_provider_;
    Resource* resource_;
    bool generate_mipmap_ = false;

    DISALLOW_COPY_AND_ASSIGN(ScopedUseGL);
  };

 protected:
  // Binds the given GL resource to a texture target for sampling using the
  // specified filter for both minification and magnification. Returns the
  // texture target used. The resource must be locked for reading.
  GLenum BindForSampling(viz::ResourceId resource_id,
                         GLenum unit,
                         GLenum filter);

 private:
  base::flat_map<viz::ResourceId, sk_sp<SkImage>> resource_sk_image_;

  DISALLOW_COPY_AND_ASSIGN(LocalResourceProvider);
};

}  // namespace cc

#endif  // CC_RESOURCES_LOCAL_RESOURCE_PROVIDER_H_
