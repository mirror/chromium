// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gpu_fence.h"

#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_fence_egl.h"
#include "ui/gl/gl_surface_egl.h"

namespace gl {

GpuFence::GpuFence() {
  DCHECK(gl::GLSurfaceEGL::IsAndroidNativeFenceSyncSupported());

  fence_ = base::MakeUnique<gl::GLFenceEGL>(EGL_SYNC_NATIVE_FENCE_ANDROID, nullptr);
  DCHECK(fence_->IsValid());
}

GpuFence::GpuFence(std::unique_ptr<GLFenceEGL> fence) {
  fence_ = std::move(fence);
}

GpuFence::~GpuFence() = default;

// static
std::unique_ptr<gl::GpuFence> GpuFence::FromHandle(const gfx::GpuFenceHandle& handle) {
  DCHECK(gl::GLSurfaceEGL::IsAndroidNativeFenceSyncSupported());
  DCHECK_EQ(handle.type, gfx::GpuFenceHandleType::FILE_DESCRIPTOR);

  DCHECK_GE(handle.native_fd.fd, 0);
  EGLint attrs[] = {EGL_SYNC_NATIVE_FENCE_FD_ANDROID, handle.native_fd.fd,
                    EGL_NONE};
  auto fence = base::MakeUnique<gl::GLFenceEGL>(EGL_SYNC_NATIVE_FENCE_ANDROID,
                                                attrs);
  if (!fence->IsValid()) {
    close(handle.native_fd.fd);
    return nullptr;
  }
  return base::WrapUnique(new GpuFence(std::move(fence)));
}

gfx::GpuFenceHandle GpuFence::GetHandle() {
  EGLint sync_fd = fence_->ExtractNativeFileDescriptor();
  if (sync_fd < 0)
    return gfx::GpuFenceHandle();

  gfx::GpuFenceHandle handle;
  handle.type = gfx::GpuFenceHandleType::FILE_DESCRIPTOR;
  handle.native_fd = base::FileDescriptor(sync_fd, /*auto_close=*/true);

  return handle;
}

std::unique_ptr<gl::GLFenceEGL> GpuFence::GetGLFence() {
  return std::move(fence_);
}

}  // namespace gl
