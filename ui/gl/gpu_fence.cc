// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gpu_fence.h"

#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_fence_egl.h"
#include "ui/gl/gl_surface_egl.h"

namespace gl {

GpuFence::GpuFence() {
  DCHECK(gl::GLSurfaceEGL::IsAndroidNativeFenceSyncSupported());

  fence_ = base::MakeUnique<gl::GLFenceEGL>(EGL_SYNC_NATIVE_FENCE_ANDROID, nullptr);
  //LOG(INFO) << __FUNCTION__ << ";;; fence_=" << fence_.get();
  DCHECK(fence_->IsValid());
}

GpuFence::GpuFence(std::unique_ptr<GLFenceEGL> fence) {
  fence_ = std::move(fence);
}

GpuFence::~GpuFence() {
  //LOG(INFO) << __FUNCTION__ << ";;; fence_=" << fence_.get();
  if (handle_.type == gfx::GpuFenceHandleType::FILE_DESCRIPTOR) {
    //LOG(INFO) << __FUNCTION__ << ";;; fd=" << handle_.native_fd.fd;
    if (handle_.native_fd.fd >= 0)
      if (IGNORE_EINTR(close(handle_.native_fd.fd)) < 0)
        PLOG(ERROR) << "close";
  }
}

// static
std::unique_ptr<gl::GpuFence> GpuFence::FromHandle(const gfx::GpuFenceHandle& handle) {
  DCHECK(gl::GLSurfaceEGL::IsAndroidNativeFenceSyncSupported());
  DCHECK_EQ(handle.type, gfx::GpuFenceHandleType::FILE_DESCRIPTOR);

  DCHECK_GE(handle.native_fd.fd, 0);
  EGLint attrs[] = {EGL_SYNC_NATIVE_FENCE_FD_ANDROID, handle.native_fd.fd,
                    EGL_NONE};
  auto fence = base::MakeUnique<gl::GLFenceEGL>(EGL_SYNC_NATIVE_FENCE_ANDROID,
                                                attrs);
  //LOG(INFO) << __FUNCTION__ << ";;; fence=" << fence.get() << " fd=" << handle.native_fd.fd;
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

  handle_ = {};
  handle_.type = gfx::GpuFenceHandleType::FILE_DESCRIPTOR;
  handle_.native_fd = base::FileDescriptor(sync_fd, /*auto_close=*/true);
  //LOG(INFO) << __FUNCTION__ << ";;; fence_=" << fence_.get() << " fd=" << handle_.native_fd.fd;

  return handle_;
}

std::unique_ptr<gl::GLFenceEGL> GpuFence::GetGLFence() {
  //LOG(INFO) << __FUNCTION__ << ";;; fence_=" << fence_.get();
  return base::WrapUnique(fence_.release());
}

}  // namespace gl
