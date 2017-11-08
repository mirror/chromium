// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_COMMON_GPU_FENCE_IMPL_H_
#define GPU_IPC_COMMON_GPU_FENCE_IMPL_H_

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "ui/gfx/gpu_fence.h"
#include "ui/gfx/gpu_fence_handle.h"
#include "gpu/gpu_export.h"

namespace gl {
class GLFence;
}

namespace gpu {

class GPU_EXPORT GpuFenceImpl : public gfx::GpuFence {
 public:
  static bool IsSupported();

  static std::unique_ptr<GpuFenceImpl> CreateFromHandle(
      const gfx::GpuFenceHandle& handle);

  static std::unique_ptr<GpuFenceImpl> CreateNew();

  virtual gl::GLFence* GetGLFence() = 0;

 protected:
  GpuFenceImpl();

 private:
  DISALLOW_COPY_AND_ASSIGN(GpuFenceImpl);
};

}  // namespace gpu

#endif  // GPU_IPC_COMMON_GPU_FENCE_IMPL_H_
