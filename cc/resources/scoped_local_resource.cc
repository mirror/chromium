// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resources/scoped_local_resource.h"
#include "base/bits.h"
#include "cc/resources/texture_id_allocator.h"
#include "components/viz/common/resources/resource_format_utils.h"
#include "gpu/GLES2/gl2extchromium.h"

namespace cc {

static GLint GetActiveTextureUnit(GLES2Interface* gl) {
  GLint active_unit = 0;
  gl->GetIntegerv(GL_ACTIVE_TEXTURE, &active_unit);
  return active_unit;
}

class ScopedSetActiveTexture {
 public:
  ScopedSetActiveTexture(GLES2Interface* gl, GLenum unit)
      : gl_(gl), unit_(unit) {
    DCHECK_EQ(GL_TEXTURE0, GetActiveTextureUnit(gl_));

    if (unit_ != GL_TEXTURE0)
      gl_->ActiveTexture(unit_);
  }

  ~ScopedSetActiveTexture() {
    // Active unit being GL_TEXTURE0 is effectively the ground state.
    if (unit_ != GL_TEXTURE0)
      gl_->ActiveTexture(GL_TEXTURE0);
  }

 private:
  GLES2Interface* gl_;
  GLenum unit_;
};

ScopedLocalResource::ScopedLocalResource(
    viz::ContextProvider* compositor_context_provider,
    TextureIdAllocator* texture_id_allocator,
    const ResourceProvider::Settings* settings)
    : compositor_context_provider_(compositor_context_provider),
      texture_id_allocator_(texture_id_allocator),
      settings_(settings) {
  DCHECK(compositor_context_provider);
  DCHECK(texture_id_allocator);
  DCHECK(settings);
  CreateTexture();
}

ScopedLocalResource::~ScopedLocalResource() {
  Free();
}

GLES2Interface* ScopedLocalResource::ContextGL() const {
  return compositor_context_provider_
             ? compositor_context_provider_->ContextGL()
             : nullptr;
}

void ScopedLocalResource::CreateTexture() {
  if (gl_id_)
    return;

  gl_id_ = texture_id_allocator_->NextId();
  DCHECK(gl_id_);

  GLES2Interface* gl = ContextGL();
  DCHECK(gl);

  // Create and set texture properties. Allocation is delayed until needed.
  gl->BindTexture(target_, gl_id_);
  gl->TexParameteri(target_, GL_TEXTURE_MIN_FILTER, original_filter_);
  gl->TexParameteri(target_, GL_TEXTURE_MAG_FILTER, original_filter_);
  gl->TexParameteri(target_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl->TexParameteri(target_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  if (settings_->use_texture_usage_hint &&
      (hint_ & viz::ResourceTextureHint::kFramebuffer)) {
    gl->TexParameteri(target_, GL_TEXTURE_USAGE_ANGLE,
                      GL_FRAMEBUFFER_ATTACHMENT_ANGLE);
  }
}

void ScopedLocalResource::Allocate(const gfx::Size& size,
                                   viz::ResourceTextureHint hint,
                                   viz::ResourceFormat format,
                                   const gfx::ColorSpace& color_space) {
  // ETC1 resources cannot be preallocated.
  if (format_ == viz::ETC1)
    return;
  if (allocated_)
    return;

  size_ = size;
  format_ = format;
  color_space_ = color_space;
  allocated_ = true;
  hint_ = hint;

  GLES2Interface* gl = ContextGL();
  DCHECK(gl);
  gl->BindTexture(target_, gl_id_);

  if (is_overlay_) {
    DCHECK(settings_->use_texture_storage_image);
    gl->TexStorage2DImageCHROMIUM(target_, viz::TextureStorageFormat(format_),
                                  GL_SCANOUT_CHROMIUM, size_.width(),
                                  size_.height());
    if (color_space_.IsValid()) {
      gl->SetColorSpaceMetadataCHROMIUM(
          gl_id_, reinterpret_cast<GLColorSpace>(&color_space_));
    }
  } else if (settings_->use_texture_storage) {
    GLint levels = 1;
    if (settings_->use_texture_npot &&
        (hint_ & viz::ResourceTextureHint::kMipmap)) {
      levels += base::bits::Log2Floor(std::max(size_.width(), size_.height()));
    }
    gl->TexStorage2DEXT(target_, levels, viz::TextureStorageFormat(format_),
                        size_.width(), size_.height());
  } else {
    gl->TexImage2D(target_, 0, GLInternalFormat(format_), size_.width(),
                   size_.height(), 0, GLDataFormat(format_),
                   GLDataType(format_), nullptr);
  }
}

void ScopedLocalResource::AllocateWithGpuMemoryBuffer(
    const gfx::Size& size,
    viz::ResourceFormat format,
    gfx::BufferUsage usage,
    const gfx::ColorSpace& color_space) {}

void ScopedLocalResource::Free() {
  if (gl_id_) {
#if DCHECK_IS_ON()
// DCHECK(allocate_thread_id_ == base::PlatformThread::CurrentId());
#endif
    DeleteResource();
  }
  allocated_ = false;
}

void ScopedLocalResource::DeleteResource() {  // DeleteStyle style
  // TRACE_EVENT0("cc", "ResourceProvider::DeleteResourceInternal");

  /*
  #if defined(OS_ANDROID)
    // If this resource was interested in promotion hints, then remove it from
    // the set of resources that we'll notify.
    if (wants_promotion_hint_)
      wants_promotion_hints_set_.erase(it->first);
  #endif
  */
  if (image_id_) {
    GLES2Interface* gl = ContextGL();
    DCHECK(gl);
    gl->DestroyImageCHROMIUM(image_id_);
    image_id_ = 0;
  }

  if (gl_id_) {
    GLES2Interface* gl = ContextGL();
    DCHECK(gl);
    gl->DeleteTextures(1, &gl_id_);
    gl_id_ = 0;
  }
}

GLenum ScopedLocalResource::BindForSampling(GLenum unit, GLenum filter) {
  // DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  GLES2Interface* gl = ContextGL();

  ScopedSetActiveTexture scoped_active_tex(gl, unit);
  gl->BindTexture(target_, gl_id_);
  GLenum min_filter = filter;

  if (filter == GL_LINEAR) {
    switch (mipmap_state_) {
      case INVALID:
        break;
      case GENERATE:
        DCHECK(settings_->use_texture_npot);
        gl->GenerateMipmap(target_);
        mipmap_state_ = VALID;
      // fall-through
      case VALID:
        min_filter = GL_LINEAR_MIPMAP_LINEAR;
        break;
    }
  }

  if (min_filter != min_filter_) {
    gl->TexParameteri(target_, GL_TEXTURE_MIN_FILTER, min_filter);
    min_filter_ = min_filter;
  }
  if (filter != filter_) {
    gl->TexParameteri(target_, GL_TEXTURE_MAG_FILTER, filter);
    filter_ = filter;
  }

  return target_;
}

}  // namespace cc
