// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_FENCE_EGL_H_
#define UI_GL_GL_FENCE_EGL_H_

#include "base/macros.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_export.h"
#include "ui/gl/gl_fence.h"

namespace gl {

class GL_EXPORT GLFenceEGL : public GLFence {
 public:
  static void SetIgnoreFailures();

  GLFenceEGL();
  explicit GLFenceEGL(bool use_default_initialization);
  ~GLFenceEGL() override;

  // Initialize with custom properties, for use on a GLFenceEGL(false)
  // constructed object. Returns true on success.
  bool Initialize(EGLenum type, EGLint* attribs);

  // GLFence implementation:
  bool HasCompleted() override;
  void ClientWait() override;
  void ServerWait() override;

  // EGL-specific wait-with-timeout implementation:
  EGLint ClientWaitWithTimeoutNanos(EGLTimeKHR timeout);

  // Extract backing file descriptor for use in a GpuFenceHandle. Requires
  // GLSurfaceEGL::IsAndroidNativeFenceSyncSupported(), and the fence must
  // have been created with type EGL_SYNC_NATIVE_FENCE_ANDROID.
  EGLint ExtractNativeFileDescriptor();

 private:
  EGLSyncKHR sync_;
  EGLDisplay display_;

  DISALLOW_COPY_AND_ASSIGN(GLFenceEGL);
};

}  // namespace gl

#endif  // UI_GL_GL_FENCE_EGL_H_
