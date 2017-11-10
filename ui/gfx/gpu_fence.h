// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GPU_FENCE_H_
#define UI_GL_GPU_FENCE_H_

#include "base/macros.h"
#include "ui/gfx/gfx_export.h"
#include "ui/gfx/gpu_fence_handle.h"

extern "C" typedef struct _ClientGpuFence* ClientGpuFence;

namespace gfx {

// This class takes ownership of a GpuFenceHandle and releases the resources in
// it when going out of scope.
class GFX_EXPORT GpuFence {
 public:
  GpuFence() = delete;
  GpuFence(const GpuFenceHandle& handle);
  ~GpuFence();

  // This handle is an unowned view of the resources owned by this class for
  // use with CloneHandleForIPC. Don't pass this to a consuming method such
  // as gl::GLFence::CreateFromGpuFenceHandle, that would cause duplicate
  // resource release.
  GpuFenceHandle GetHandle();

  // Casts for use with the GLES interface.
  ClientGpuFence AsClientGpuFence();
  static GpuFence* FromClientGpuFence(ClientGpuFence gpu_fence);

 private:
  GpuFenceHandle handle_;

  DISALLOW_COPY_AND_ASSIGN(GpuFence);
};

}  // namespace gfx

#endif  // UI_GL_GPU_FENCE_H_
