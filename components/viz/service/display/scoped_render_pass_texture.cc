// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display/scoped_render_pass_texture.h"

#include "base/logging.h"
#include "components/viz/common/resources/resource_format_utils.h"
#include "gpu/GLES2/gl2extchromium.h"

using gpu::gles2::GLES2Interface;

namespace viz {

ScopedRenderPassTexture::ScopedRenderPassTexture(
    GLES2Interface* gl,
    const bool use_texture_usage_hint,
    const bool use_texture_storage)
    : gl_(gl),
      use_texture_usage_hint_(use_texture_usage_hint),
      use_texture_storage_(use_texture_storage) {
  DCHECK(gl_);
}

ScopedRenderPassTexture::~ScopedRenderPassTexture() {
  Free();
}

void ScopedRenderPassTexture::CreateTexture() {
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
  if (use_texture_usage_hint_) {
    gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_USAGE_ANGLE,
                       GL_FRAMEBUFFER_ATTACHMENT_ANGLE);
  }
}

void ScopedRenderPassTexture::Allocate(const gfx::Size& size) {
  if (allocated_)
    return;

  if (!gl_id_)
    CreateTexture();

  size_ = size;
  allocated_ = true;

  gl_->BindTexture(GL_TEXTURE_2D, gl_id_);

  if (use_texture_storage_) {
    GLint levels = 1;
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

  allocated_ = false;
}

void ScopedRenderPassTexture::BindForSampling() {
  gl_->BindTexture(GL_TEXTURE_2D, gl_id_);
}

}  // namespace viz
