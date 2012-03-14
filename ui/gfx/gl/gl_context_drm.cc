// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/gl/gl_context_egl.h"

#include "base/logging.h"
#include "ui/gfx/gl/gl_bindings.h"
#include "ui/gfx/gl/gl_implementation.h"

namespace gfx {

// Encapsulates an EGL OpenGL ES context.
class GLContextDRM : public GLContextEGL {
 public:
  explicit GLContextDRM(GLShareGroup* share_group);
  virtual ~GLContextDRM();

  // Implement GLContext.
  virtual void SetSwapInterval(int interval) OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(GLContextDRM);
};

GLContextDRM::GLContextDRM(GLShareGroup* share_group)
    : GLContextEGL(share_group) {
}

GLContextDRM::~GLContextDRM() {
}

void GLContextDRM::SetSwapInterval(int interval) {
}

// static
scoped_refptr<GLContext> GLContext::CreateGLContext(
    GLShareGroup* share_group,
    GLSurface* compatible_surface,
    GpuPreference gpu_preference) {

  scoped_refptr<GLContext> context(new GLContextDRM(share_group));
  if (!context->Initialize(compatible_surface, gpu_preference))
    return NULL;
  return context;
}

bool GLContext::SupportsDualGpus() {
  return false;
}

}
