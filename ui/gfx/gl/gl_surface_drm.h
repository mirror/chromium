// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_GL_GL_SURFACE_DRM_H_
#define UI_GFX_GL_GL_SURFACE_DRM_H_
#pragma once

#include "ui/gfx/gl/gl_surface_egl.h"

struct gbm_surface;

namespace gfx {

// Encapsulates a pixmap EGL surface.
class GLSurfaceDRM : public GLSurfaceEGL {
 public:
  GLSurfaceDRM(const Size& size);
  virtual ~GLSurfaceDRM();

  // Implement GLSurface.
  virtual bool Initialize() OVERRIDE;
  virtual void Destroy() OVERRIDE;
  virtual bool Resize(const Size& size) OVERRIDE;
  virtual bool IsOffscreen() OVERRIDE;
  virtual bool SwapBuffers() OVERRIDE;
  virtual Size GetSize() OVERRIDE;
  virtual EGLSurface GetHandle() OVERRIDE;

 protected:
  Size size_;
  EGLSurface surface_;
  gbm_surface *native_surface_;
  bool is_offscreen_;

  DISALLOW_COPY_AND_ASSIGN(GLSurfaceDRM);
};

}  // namespace gfx

#endif  // UI_GFX_GL_GL_SURFACE_DRM_H_
