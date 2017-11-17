// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_fence_egl.h"

#include "ui/gl/egl_util.h"
#include "ui/gl/gl_bindings.h"

namespace gl {

namespace {

bool g_ignore_egl_sync_failures = false;

}  // namespace

// static
void GLFenceEGL::SetIgnoreFailures() {
  g_ignore_egl_sync_failures = true;
}

GLFenceEGL::GLFenceEGL() = default;

// static
std::unique_ptr<GLFenceEGL> GLFenceEGL::Create() {
  auto fence = Create(EGL_SYNC_FENCE_KHR, nullptr);
  // Default creation isn't supposed to fail.
  DCHECK(fence);
  return fence;
}

// static
std::unique_ptr<GLFenceEGL> GLFenceEGL::Create(EGLenum type, EGLint* attribs) {
  // Must use a helper class, the no-args constructor is protected.
  struct GLFenceEGLHelper : public GLFenceEGL {};
  auto fence = base::MakeUnique<GLFenceEGLHelper>();

  bool success = fence->Initialize(type, attribs);
  if (success)
    return fence;
  return nullptr;
}

bool GLFenceEGL::Initialize(EGLenum type, EGLint* attribs) {
  display_ = eglGetCurrentDisplay();
  sync_ = eglCreateSyncKHR(display_, type, attribs);
  glFlush();
  return sync_ != EGL_NO_SYNC_KHR;
}

bool GLFenceEGL::HasCompleted() {
  EGLint value = 0;
  if (eglGetSyncAttribKHR(display_, sync_, EGL_SYNC_STATUS_KHR, &value) !=
      EGL_TRUE) {
    LOG(ERROR) << "Failed to get EGLSync attribute. error code:"
               << eglGetError();
    return true;
  }

  DCHECK(value == EGL_SIGNALED_KHR || value == EGL_UNSIGNALED_KHR);
  return !value || value == EGL_SIGNALED_KHR;
}

void GLFenceEGL::ClientWait() {
  EGLint result = ClientWaitWithTimeoutNanos(EGL_FOREVER_KHR);
  DCHECK(g_ignore_egl_sync_failures || EGL_TIMEOUT_EXPIRED_KHR != result);
}

EGLint GLFenceEGL::ClientWaitWithTimeoutNanos(EGLTimeKHR timeout) {
  EGLint flags = 0;
  EGLint result = eglClientWaitSyncKHR(display_, sync_, flags, timeout);
  if (result == EGL_FALSE) {
    LOG(ERROR) << "Failed to wait for EGLSync. error:"
               << ui::GetLastEGLErrorString();
    CHECK(g_ignore_egl_sync_failures);
  }
  return result;
}

void GLFenceEGL::ServerWait() {
  if (!g_driver_egl.ext.b_EGL_KHR_wait_sync) {
    ClientWait();
    return;
  }
  EGLint flags = 0;
  if (eglWaitSyncKHR(display_, sync_, flags) == EGL_FALSE) {
    LOG(ERROR) << "Failed to wait for EGLSync. error:"
               << ui::GetLastEGLErrorString();
    CHECK(g_ignore_egl_sync_failures);
  }
}

GLFenceEGL::~GLFenceEGL() {
  eglDestroySyncKHR(display_, sync_);
}

}  // namespace gl
