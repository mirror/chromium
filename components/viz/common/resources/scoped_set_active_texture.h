// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCES_SCOPED_SET_ACTIVE_TEXTURE_H_
#define COMPONENTS_VIZ_COMMON_RESOURCES_SCOPED_SET_ACTIVE_TEXTURE_H_

#include "components/viz/common/viz_common_export.h"
#include "gpu/command_buffer/client/gles2_interface.h"

namespace viz {

static GLint GetActiveTextureUnit(gpu::gles2::GLES2Interface* gl) {
  GLint active_unit = 0;
  gl->GetIntegerv(GL_ACTIVE_TEXTURE, &active_unit);
  return active_unit;
}

class VIZ_COMMON_EXPORT ScopedSetActiveTexture {
 public:
  ScopedSetActiveTexture(gpu::gles2::GLES2Interface* gl, GLenum unit)
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
  gpu::gles2::GLES2Interface* gl_;
  GLenum unit_;
  DISALLOW_COPY_AND_ASSIGN(ScopedSetActiveTexture);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCES_SCOPED_SET_ACTIVE_TEXTURE_H_
