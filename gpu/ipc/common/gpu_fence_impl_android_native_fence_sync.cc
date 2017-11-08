// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/common/gpu_fence_impl_android_native_fence_sync.h"

#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "ui/gl/gl_fence_egl.h"

namespace gpu {

GpuFenceImplAndroidNativeFenceSync::GpuFenceImplAndroidNativeFenceSync(
    const gfx::GpuFenceHandle& handle) {
  DCHECK_GE(handle.native_fd.fd, 0);
  EGLint attrs[] = {EGL_SYNC_NATIVE_FENCE_FD_ANDROID, handle.native_fd.fd,
                    EGL_NONE};
  fence_ = base::MakeUnique<gl::GLFenceEGL>(
      EGL_SYNC_NATIVE_FENCE_ANDROID, attrs);
  //LOG(INFO) << __FUNCTION__ << ";;; fence_=" << fence_.get();
  DCHECK(fence_->IsValid());
}

GpuFenceImplAndroidNativeFenceSync::GpuFenceImplAndroidNativeFenceSync() {
  fence_ = base::MakeUnique<gl::GLFenceEGL>(EGL_SYNC_NATIVE_FENCE_ANDROID,
                                            nullptr);
  DCHECK(fence_->IsValid());
}

GpuFenceImplAndroidNativeFenceSync::~GpuFenceImplAndroidNativeFenceSync() {
  //LOG(INFO) << __FUNCTION__ << ";;; fence_=" << fence_.get();
  if (handle_.type == gfx::GpuFenceHandleType::ANDROID_NATIVE_FENCE_SYNC) {
    //LOG(INFO) << __FUNCTION__ << ";;; fd=" << handle_.native_fd.fd;
    if (handle_.native_fd.fd >= 0)
      if (IGNORE_EINTR(close(handle_.native_fd.fd)) < 0)
        PLOG(ERROR) << "close";
  }
}

gl::GLFence* GpuFenceImplAndroidNativeFenceSync::GetGLFence() {
  //LOG(INFO) << __FUNCTION__ << ";;; fence_=" << fence_.get();
  return fence_.get();
}

gfx::GpuFenceHandle GpuFenceImplAndroidNativeFenceSync::GetHandle() {
  EGLint sync_fd = fence_->ExtractNativeFileDescriptor();
  if (sync_fd < 0)
    return gfx::GpuFenceHandle();

  handle_ = {};
  handle_.type = gfx::GpuFenceHandleType::ANDROID_NATIVE_FENCE_SYNC;
  handle_.native_fd = base::FileDescriptor(sync_fd, /*auto_close=*/true);
  //LOG(INFO) << __FUNCTION__ << ";;; fence_=" << fence_.get() << " fd=" << handle_.native_fd.fd;

  return handle_;
}

}  // namespace gpu
