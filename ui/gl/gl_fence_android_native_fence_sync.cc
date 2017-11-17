// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_fence_android_native_fence_sync.h"

#include "ui/gfx/gpu_fence_handle.h"
#include "ui/gl/gl_surface_egl.h"

#if defined(OS_POSIX)
#include <unistd.h>

#include "base/posix/eintr_wrapper.h"
#endif

namespace gl {

GLFenceAndroidNativeFenceSync::GLFenceAndroidNativeFenceSync() {}

GLFenceAndroidNativeFenceSync::~GLFenceAndroidNativeFenceSync() {}

// static
std::unique_ptr<GLFenceAndroidNativeFenceSync>
GLFenceAndroidNativeFenceSync::CreateInternal(EGLenum type, EGLint* attribs) {
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
GLFenceAndroidNativeFenceSync::CreateGpuFence() {
  auto fence = CreateInternal(EGL_SYNC_NATIVE_FENCE_ANDROID, nullptr);
  if (!fence)
    return nullptr;
  EGLint sync_fd = eglDupNativeFenceFDANDROID(fence->display_, fence->sync_);
  if (sync_fd < 0)
    return nullptr;
  fence->owned_fd_.reset(sync_fd);
  return fence;
}

// static
std::unique_ptr<GLFenceAndroidNativeFenceSync>
GLFenceAndroidNativeFenceSync::CreateFromGpuFenceHandle(
    const gfx::GpuFenceHandle& handle) {
  DCHECK_GE(handle.native_fd.fd, 0);
  EGLint attribs[] = {EGL_SYNC_NATIVE_FENCE_FD_ANDROID, handle.native_fd.fd,
                      EGL_NONE};
  auto fence = CreateInternal(EGL_SYNC_NATIVE_FENCE_ANDROID, attribs);
  DCHECK(fence);
  return fence;
}

gfx::GpuFenceHandle GLFenceAndroidNativeFenceSync::GetGpuFenceHandle() {
  DCHECK(GLSurfaceEGL::IsAndroidNativeFenceSyncSupported());
  gfx::GpuFenceHandle handle;
  handle.type = gfx::GpuFenceHandleType::ANDROID_NATIVE_FENCE_SYNC;

  // If this object was constructed as a new GpuFence, we already have a
  // duplicated handle. Return it.
  if (owned_fd_.is_valid()) {
    handle.native_fd = base::FileDescriptor(owned_fd_.get(),
                                            /*auto_close=*/false);
    return handle;
  }

  // Otherwise, if this was constructed from a handle, we've already
  // consumed the input handle's FD. Duplicate a fresh one and save it.
  EGLint sync_fd = eglDupNativeFenceFDANDROID(display_, sync_);
  if (sync_fd < 0)
    return gfx::GpuFenceHandle();
  owned_fd_.reset(sync_fd);
  handle.native_fd =
      base::FileDescriptor(owned_fd_.get(), /*auto_close=*/false);
  return handle;
}

}  // namespace gl
