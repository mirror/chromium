// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_fence_android_native_fence_sync.h"

#include "ui/gfx/gpu_fence_handle.h"

namespace gl {

GLFenceAndroidNativeFenceSync::GLFenceAndroidNativeFenceSync()
    : GLFenceEGL(false) {
  bool success = Initialize(EGL_SYNC_NATIVE_FENCE_ANDROID, nullptr);
  DCHECK(success);
}

// static
std::unique_ptr<GLFenceEGL>
GLFenceAndroidNativeFenceSync::CreateFromGpuFenceHandle(
    const gfx::GpuFenceHandle& handle) {
  DCHECK_GE(handle.native_fd.fd, 0);
  EGLint attrs[] = {EGL_SYNC_NATIVE_FENCE_FD_ANDROID, handle.native_fd.fd,
                    EGL_NONE};
  auto fence = base::MakeUnique<gl::GLFenceEGL>(false);
  bool success = fence->Initialize(EGL_SYNC_NATIVE_FENCE_ANDROID, attrs);
  // LOG(INFO) << __FUNCTION__ << ";;; fence=" << fence.get();
  DCHECK(success);
  return fence;
}

gfx::GpuFenceHandle GLFenceAndroidNativeFenceSync::GetGpuFenceHandle() {
  EGLint sync_fd = ExtractNativeFileDescriptor();
  if (sync_fd < 0)
    return gfx::GpuFenceHandle();

  gfx::GpuFenceHandle handle;
  handle.type = gfx::GpuFenceHandleType::ANDROID_NATIVE_FENCE_SYNC;
  handle.native_fd = base::FileDescriptor(sync_fd, /*auto_close=*/true);
  // LOG(INFO) << __FUNCTION__ << ";;; fence_=" << fence_.get() << " fd=" <<
  // handle_.native_fd.fd;

  return handle;
}

}  // namespace gl
