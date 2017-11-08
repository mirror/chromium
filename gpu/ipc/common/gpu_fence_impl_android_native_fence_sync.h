// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_COMMON_GPU_FENCE_IMPL_ANDROID_NATIVE_FENCE_SYNC_H_
#define GPU_IPC_COMMON_GPU_FENCE_IMPL_ANDROID_NATIVE_FENCE_SYNC_H_

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "ui/gfx/gpu_fence.h"
#include "ui/gfx/gpu_fence_handle.h"
#include "gpu/gpu_export.h"
#include "gpu/ipc/common/gpu_fence_impl.h"

namespace gl {
class GLFenceEGL;
}

namespace gpu {

class GPU_EXPORT GpuFenceImplAndroidNativeFenceSync : public GpuFenceImpl {
 public:
  GpuFenceImplAndroidNativeFenceSync();
  GpuFenceImplAndroidNativeFenceSync(const gfx::GpuFenceHandle& handle);
  ~GpuFenceImplAndroidNativeFenceSync() override;

  gfx::GpuFenceHandle GetHandle() override;

  gl::GLFence* GetGLFence() override;

 private:
  std::unique_ptr<gl::GLFenceEGL> fence_;

  // The handle starts out empty and is populated on a GetHandle() call. This
  // object owns it and will close file descriptors on destruction. Use
  // CloneHandleForIPC if you need longer lifetime.
  gfx::GpuFenceHandle handle_;

  DISALLOW_COPY_AND_ASSIGN(GpuFenceImplAndroidNativeFenceSync);
};

}  // namespace gpu

#endif  // GPU_IPC_COMMON_GPU_FENCE_IMPL_ANDROID_NATIVE_FENCE_SYNC_H_
