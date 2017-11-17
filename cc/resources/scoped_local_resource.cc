// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resources/scoped_local_resource.h"
#include "base/bits.h"
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
    GLES2Interface* gl,
    const viz::ResourceTextureSettings* settings)
    : gl_(gl), settings_(settings) {
  DCHECK(gl_);
  DCHECK(settings_);
}

ScopedLocalResource::~ScopedLocalResource() {
  Free();
}

void ScopedLocalResource::CreateTexture() {
  if (gl_id_)
    return;

  gl_->GenTextures(1, &gl_id_);
  DCHECK(gl_id_);

  // Create and set texture properties.
  gl_->BindTexture(GL_TEXTURE_2D, gl_id_);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  if (settings_->use_texture_usage_hint) {
    gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_USAGE_ANGLE,
                       GL_FRAMEBUFFER_ATTACHMENT_ANGLE);
  }
}

void ScopedLocalResource::Allocate(const gfx::Size& size,
                                   viz::ResourceFormat format,
                                   const gfx::ColorSpace& color_space) {
  if (allocated_)
    return;

  if (!gl_id_)
    CreateTexture();

  size_ = size;
  color_space_ = color_space;
  allocated_ = true;

  gl_->BindTexture(GL_TEXTURE_2D, gl_id_);

  if (settings_->use_texture_storage) {
    GLint levels = 1;
    gl_->TexStorage2DEXT(
        GL_TEXTURE_2D, levels,
        viz::TextureStorageFormat(viz::ResourceFormat::RGBA_8888),
        size_.width(), size_.height());
  } else {
    gl_->TexImage2D(GL_TEXTURE_2D, 0,
                    GLInternalFormat(viz::ResourceFormat::RGBA_8888),
                    size_.width(), size_.height(), 0,
                    GLDataFormat(viz::ResourceFormat::RGBA_8888),
                    GLDataType(viz::ResourceFormat::RGBA_8888), nullptr);
  }
}

void ScopedLocalResource::Free() {
  if (!gl_id_)
    return;

  gl_->DeleteTextures(1, &gl_id_);
  gl_id_ = 0;

  allocated_ = false;
}

GLenum ScopedLocalResource::BindForSampling(GLenum unit, GLenum filter) {
  ScopedSetActiveTexture scoped_active_tex(gl_, unit);
  gl_->BindTexture(GL_TEXTURE_2D, gl_id_);
  return GL_TEXTURE_2D;
}

}  // namespace cc
