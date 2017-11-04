// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GPU_FENCE_H_
#define UI_GL_GPU_FENCE_H_

#include "base/macros.h"
#include "ui/gfx/gpu_fence_handle.h"
#include "ui/gl/gl_fence_egl.h"

namespace gl {

class GL_EXPORT GpuFence {
 public:
  static bool IsSupported();

  GpuFence();
  ~GpuFence();

  static std::unique_ptr<GpuFence> FromHandle(const gfx::GpuFenceHandle& handle);
  std::unique_ptr<GLFenceEGL> GetGLFence();
  gfx::GpuFenceHandle GetHandle();

 private:
  GpuFence(std::unique_ptr<GLFenceEGL> fence);

  std::unique_ptr<GLFenceEGL> fence_;
  // The handle is only populated after GetHandle, this object owns it
  // and will close file descriptors on destruction. Use CloneHandleForIPC if
  // you need longer lifetime.
  gfx::GpuFenceHandle handle_;

  DISALLOW_COPY_AND_ASSIGN(GpuFence);
};

}  // namespace gl

#endif  // UI_GL_GPU_FENCE_H_
