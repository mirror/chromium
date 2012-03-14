// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_GL_GL_SURFACE_DRM_H_
#define UI_GFX_GL_GL_SURFACE_DRM_H_
#pragma once

#include "third_party/angle/include/EGL/egl.h"

#include "base/compiler_specific.h"
#include "ui/gfx/gl/gl_surface.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/size.h"

namespace gfx {

// Encapsulates a pixmap EGL surface.
class GLSurfaceDRM : public GLSurface {
 public:
  explicit GLSurfaceDRM(const Size& size);
  explicit GLSurfaceDRM();
  virtual ~GLSurfaceDRM();

  // Implement GLSurface.
  virtual EGLDisplay GetDisplay() OVERRIDE;
  virtual EGLConfig GetConfig() OVERRIDE;
  virtual bool Initialize() OVERRIDE;
  virtual void Destroy() OVERRIDE;
  virtual bool Resize(const Size& size) OVERRIDE;
  virtual bool IsOffscreen() OVERRIDE;
  virtual bool SwapBuffers() OVERRIDE;
  virtual Size GetSize() OVERRIDE;
  virtual EGLSurface GetHandle() OVERRIDE;

  static bool InitializeOneOff();

  static EGLDisplay GetHardwareDisplay();
  static EGLNativeDisplayType GetNativeDisplay();

 protected:
  Size size_;

  DISALLOW_COPY_AND_ASSIGN(GLSurfaceDRM);
};

}  // namespace gfx

#endif  // UI_GFX_GL_GL_SURFACE_DRM_H_
