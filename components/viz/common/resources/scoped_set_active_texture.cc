// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/resources/scoped_set_active_texture.h"

#include "base/logging.h"
#include "gpu/command_buffer/client/gles2_interface.h"

using gpu::gles2::GLES2Interface;

namespace viz {

static GLint GetActiveTextureUnit(GLES2Interface* gl) {
  GLint active_unit = 0;
  gl->GetIntegerv(GL_ACTIVE_TEXTURE, &active_unit);
  return active_unit;
}

ScopedSetActiveTexture::ScopedSetActiveTexture(GLES2Interface* gl, GLenum unit)
    : gl_(gl), unit_(unit) {
  DCHECK_EQ(GL_TEXTURE0, GetActiveTextureUnit(gl_));

  if (unit_ != GL_TEXTURE0)
    gl_->ActiveTexture(unit_);
}

ScopedSetActiveTexture::~ScopedSetActiveTexture() {
  // Active unit being GL_TEXTURE0 is effectively the ground state.
  if (unit_ != GL_TEXTURE0)
    gl_->ActiveTexture(GL_TEXTURE0);
}

}  // namespace viz
