// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display/scoped_render_pass_texture.h"

#include "base/bits.h"
#include "base/logging.h"
#include "components/viz/common/resources/resource_format_utils.h"
#include "gpu/GLES2/gl2extchromium.h"

using gpu::gles2::GLES2Interface;

namespace viz {

ScopedRenderPassTexture::ScopedRenderPassTexture(
    GLES2Interface* gl,
    const gfx::Size& size,
    ResourceTextureHint hint,
    const bool use_texture_usage_hint,
    const bool use_texture_storage,
    const bool use_texture_npot)
    : gl_(gl),
      size_(size),
      hint_(hint),
      use_texture_usage_hint_(use_texture_usage_hint),
      use_texture_storage_(use_texture_storage),
      use_texture_npot_(use_texture_npot) {
  DCHECK(gl_);
  CreateTexture();
  Allocate();
}

ScopedRenderPassTexture::ScopedRenderPassTexture(
    GLES2Interface* gl,
    GLuint gl_id,
    const gfx::Size& size,
    ResourceTextureHint hint,
    const bool use_texture_usage_hint,
    const bool use_texture_storage,
    const bool use_texture_npot)
    : gl_(gl),
      gl_id_(gl_id),
      size_(size),
      hint_(hint),
      use_texture_usage_hint_(use_texture_usage_hint),
      use_texture_storage_(use_texture_storage),
      use_texture_npot_(use_texture_npot) {
  DCHECK(gl_);
  if (!gl_id_)
    CreateTexture();
  Allocate();
}

ScopedRenderPassTexture::ScopedRenderPassTexture(
    ScopedRenderPassTexture&& other) {
  gl_ = other.gl_;
  size_ = other.size_;
  hint_ = other.hint_;
  use_texture_usage_hint_ = other.use_texture_usage_hint_;
  use_texture_storage_ = other.use_texture_storage_;
  use_texture_npot_ = other.use_texture_npot_;
  gl_id_ = other.gl_id_;
  mipmap_state_ = other.mipmap_state_;
  min_filter_ = other.min_filter_;

  // After being moved, other will no longer keep this gl_id_.
  other.gl_id_ = 0;
}

ScopedRenderPassTexture::~ScopedRenderPassTexture() {
  Free();
}

ScopedRenderPassTexture& ScopedRenderPassTexture::operator=(
    ScopedRenderPassTexture&& other) {
  if (this != &other) {
    Free();
    gl_ = other.gl_;
    size_ = other.size_;
    hint_ = other.hint_;
    use_texture_usage_hint_ = other.use_texture_usage_hint_;
    use_texture_storage_ = other.use_texture_storage_;
    use_texture_npot_ = other.use_texture_npot_;
    gl_id_ = other.gl_id_;
    mipmap_state_ = other.mipmap_state_;
    min_filter_ = other.min_filter_;
    // After being moved, other will no longer keep this gl_id_.
    other.gl_id_ = 0;
  }
  return *this;
}

void ScopedRenderPassTexture::CreateTexture() {
  gl_->GenTextures(1, &gl_id_);
  DCHECK(gl_id_);

  // Create and set texture properties.
  gl_->BindTexture(GL_TEXTURE_2D, gl_id_);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  if (use_texture_usage_hint_ && (hint_ & ResourceTextureHint::kFramebuffer)) {
    gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_USAGE_ANGLE,
                       GL_FRAMEBUFFER_ATTACHMENT_ANGLE);
  }
}

void ScopedRenderPassTexture::Allocate() {
  Allocate(size_, hint_);
}

void ScopedRenderPassTexture::Allocate(const gfx::Size& size,
                                       ResourceTextureHint hint) {
  size_ = size;
  hint_ = hint;
  gl_->BindTexture(GL_TEXTURE_2D, gl_id_);

  if (use_texture_storage_) {
    GLint levels = 1;
    if (use_texture_npot_ && (hint_ & ResourceTextureHint::kMipmap)) {
      levels += base::bits::Log2Floor(std::max(size_.width(), size_.height()));
    }

    gl_->TexStorage2DEXT(GL_TEXTURE_2D, levels,
                         TextureStorageFormat(ResourceFormat::RGBA_8888),
                         size_.width(), size_.height());
  } else {
    gl_->TexImage2D(GL_TEXTURE_2D, 0,
                    GLInternalFormat(ResourceFormat::RGBA_8888), size_.width(),
                    size_.height(), 0, GLDataFormat(ResourceFormat::RGBA_8888),
                    GLDataType(ResourceFormat::RGBA_8888), nullptr);
  }
}

void ScopedRenderPassTexture::Free() {
  if (!gl_id_)
    return;
  gl_->DeleteTextures(1, &gl_id_);
  gl_id_ = 0;
}

void ScopedRenderPassTexture::BindForSampling() {
  gl_->BindTexture(GL_TEXTURE_2D, gl_id_);
  GLenum min_filter = GL_LINEAR;
  switch (mipmap_state_) {
    case INVALID:
      break;
    case GENERATE:
      DCHECK(use_texture_npot_);
      gl_->GenerateMipmap(GL_TEXTURE_2D);
      mipmap_state_ = VALID;
    // fall-through
    case VALID:
      min_filter = GL_LINEAR_MIPMAP_LINEAR;
      break;
  }
  if (min_filter != min_filter_) {
    gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    min_filter_ = min_filter;
  }
}

}  // namespace viz
