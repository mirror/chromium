// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resources/local_resource_provider.h"

#include "base/trace_event/trace_event.h"
#include "components/viz/common/resources/resource_format_utils.h"
#include "components/viz/common/resources/shared_bitmap_manager.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"

using gpu::gles2::GLES2Interface;

namespace cc {

LocalResourceProvider::LocalResourceProvider(
    viz::ContextProvider* compositor_context_provider,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    const viz::ResourceSettings& resource_settings)
    : ResourceProvider(compositor_context_provider,
                       nullptr,
                       gpu_memory_buffer_manager,
                       false,
                       resource_settings) {}

LocalResourceProvider::~LocalResourceProvider() = default;

GLenum LocalResourceProvider::BindForSampling(viz::ResourceId resource_id,
                                              GLenum unit,
                                              GLenum filter) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  GLES2Interface* gl = ContextGL();
  ResourceMap::iterator it = resources_.find(resource_id);
  DCHECK(it != resources_.end());
  Resource* resource = &it->second;

  ResourceProvider::ScopedSetActiveTexture scoped_active_tex(gl, unit);
  GLenum target = resource->target;
  gl->BindTexture(target, resource->gl_id);
  GLenum min_filter = filter;
  if (filter == GL_LINEAR) {
    switch (resource->mipmap_state) {
      case Resource::INVALID:
        break;
      case Resource::GENERATE:
        DCHECK(settings_.use_texture_npot);
        gl->GenerateMipmap(target);
        resource->mipmap_state = Resource::VALID;
      // fall-through
      case Resource::VALID:
        min_filter = GL_LINEAR_MIPMAP_LINEAR;
        break;
    }
  }
  if (min_filter != resource->min_filter) {
    gl->TexParameteri(target, GL_TEXTURE_MIN_FILTER, min_filter);
    resource->min_filter = min_filter;
  }
  if (filter != resource->filter) {
    gl->TexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
    resource->filter = filter;
  }

  return target;
}

LocalResourceProvider::ScopedReadGL::ScopedReadGL(
    LocalResourceProvider* resource_provider,
    viz::ResourceId resource_id) {
  resource_ = resource_provider->GetResource(resource_id);
  DCHECK(resource_);
  DCHECK(resource_->allocated);
}

LocalResourceProvider::ScopedReadGL::~ScopedReadGL() = default;

LocalResourceProvider::ScopedSamplerGL::ScopedSamplerGL(
    LocalResourceProvider* resource_provider,
    viz::ResourceId resource_id,
    GLenum filter)
    : resource_lock_(resource_provider, resource_id),
      unit_(GL_TEXTURE0),
      target_(resource_provider->BindForSampling(resource_id, unit_, filter)) {}

LocalResourceProvider::ScopedSamplerGL::ScopedSamplerGL(
    LocalResourceProvider* resource_provider,
    viz::ResourceId resource_id,
    GLenum unit,
    GLenum filter)
    : resource_lock_(resource_provider, resource_id),
      unit_(unit),
      target_(resource_provider->BindForSampling(resource_id, unit_, filter)) {}

LocalResourceProvider::ScopedSamplerGL::~ScopedSamplerGL() = default;

LocalResourceProvider::ScopedUseGL::ScopedUseGL(
    LocalResourceProvider* resource_provider,
    viz::ResourceId resource_id)
    : resource_provider_(resource_provider) {
  resource_ = resource_provider->GetResource(resource_id);
  DCHECK(resource_);
  DCHECK(IsGpuResourceType(resource_->type));
  resource_provider->CreateTexture(resource_);

  if (resource_->allocated)
    return;
  resource_->allocated = true;
  if (resource_->type == RESOURCE_TYPE_GPU_MEMORY_BUFFER) {
    AllocateGpuMemoryBuffer(resource_provider_->ContextGL(), resource_->gl_id);
  } else {
    AllocateTexture(resource_provider_->ContextGL(), resource_->gl_id);
  }
}

LocalResourceProvider::ScopedUseGL::~ScopedUseGL() {
  DCHECK(resource_);
  if (generate_mipmap_)
    resource_->SetGenerateMipmap();
}

void LocalResourceProvider::ScopedUseGL::AllocateGpuMemoryBuffer(
    GLES2Interface* gl,
    GLuint texture_id) {
  DCHECK(resource_);
  DCHECK(!resource_->gpu_memory_buffer);
  DCHECK(!resource_->image_id);

  resource_->gpu_memory_buffer =
      resource_provider_->gpu_memory_buffer_manager()->CreateGpuMemoryBuffer(
          resource_->size, BufferFormat(resource_->format), resource_->usage,
          gpu::kNullSurfaceHandle);
  // Avoid crashing in release builds if GpuMemoryBuffer allocation fails.
  // http://crbug.com/554541
  if (!resource_->gpu_memory_buffer)
    return;

  if (resource_->color_space.IsValid()) {
    resource_->gpu_memory_buffer->SetColorSpaceForScanout(
        resource_->color_space);
  }

#if defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
  // TODO(reveman): This avoids a performance problem on ARM ChromeOS
  // devices. This only works with shared memory backed buffers.
  // crbug.com/580166
  DCHECK_EQ(resource_->gpu_memory_buffer->GetHandle().type,
            gfx::SHARED_MEMORY_BUFFER);
#endif

  resource_->image_id = gl->CreateImageCHROMIUM(
      resource_->gpu_memory_buffer->AsClientBuffer(), resource_->size.width(),
      resource_->size.height(), GLInternalFormat(resource_->format));
  DCHECK(resource_->image_id || gl->GetGraphicsResetStatusKHR() != GL_NO_ERROR);
  gl->BindTexture(resource_->target, texture_id);
  gl->BindTexImage2DCHROMIUM(resource_->target, resource_->image_id);
}

void LocalResourceProvider::ScopedUseGL::AllocateTexture(GLES2Interface* gl,
                                                         GLuint texture_id) {
  DCHECK(gl);
  DCHECK(resource_);
  DCHECK(resource_->format != viz::ETC1);
  gl->BindTexture(resource_->target, texture_id);
  gl->TexImage2D(resource_->target, 0, GLInternalFormat(resource_->format),
                 resource_->size.width(), resource_->size.height(), 0,
                 GLDataFormat(resource_->format), GLDataType(resource_->format),
                 nullptr);
}

}  // namespace cc
