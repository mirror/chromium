// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_fence_android_native_fence_sync.h"

#include "ui/gfx/gpu_fence_handle.h"
#include "ui/gl/gl_surface_egl.h"

namespace gl {

GLFenceAndroidNativeFenceSync::GLFenceAndroidNativeFenceSync() {}

GLFenceAndroidNativeFenceSync::~GLFenceAndroidNativeFenceSync() {}

// static
std::unique_ptr<GLFenceAndroidNativeFenceSync>
GLFenceAndroidNativeFenceSync::Create(EGLenum type, EGLint* attribs) {
  DCHECK(GLSurfaceEGL::IsAndroidNativeFenceSyncSupported());

  // Must use a helper class, the no-args constructor is private.
  struct GLFenceAndroidNativeFenceSyncHelper
      : public GLFenceAndroidNativeFenceSync {};
  auto fence = base::MakeUnique<GLFenceAndroidNativeFenceSyncHelper>();

  bool success = fence->Initialize(type, attribs);
  if (success)
    return fence;
  return nullptr;
}

// static
std::unique_ptr<GLFenceAndroidNativeFenceSync>
GLFenceAndroidNativeFenceSync::CreateForGpuFence() {
  return Create(EGL_SYNC_NATIVE_FENCE_ANDROID, nullptr);
}

// static
std::unique_ptr<GLFenceAndroidNativeFenceSync>
GLFenceAndroidNativeFenceSync::CreateFromGpuFenceHandle(
    const gfx::GpuFenceHandle& handle) {
  DCHECK_GE(handle.native_fd.fd, 0);
  EGLint attribs[] = {EGL_SYNC_NATIVE_FENCE_FD_ANDROID, handle.native_fd.fd,
                      EGL_NONE};
  return Create(EGL_SYNC_NATIVE_FENCE_ANDROID, attribs);
}

gfx::GpuFenceHandle GLFenceAndroidNativeFenceSync::GetGpuFenceHandle() {
  DCHECK(GLSurfaceEGL::IsAndroidNativeFenceSyncSupported());
  EGLint sync_fd = eglDupNativeFenceFDANDROID(display_, sync_);
  if (sync_fd < 0)
    return gfx::GpuFenceHandle();

  gfx::GpuFenceHandle handle;
  handle.type = gfx::GpuFenceHandleType::ANDROID_NATIVE_FENCE_SYNC;
  handle.native_fd = base::FileDescriptor(sync_fd, /*auto_close=*/true);

  return handle;
}

}  // namespace gl
