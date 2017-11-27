// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GPU_FENCE_PASSTHROUGH_H_
#define UI_GL_GPU_FENCE_PASSTHROUGH_H_

#include "base/macros.h"
#include "build/build_config.h"
#include "ui/gfx/gfx_export.h"
#include "ui/gfx/gpu_fence.h"

namespace gfx {

// Implementation of GpuFence for passing a received GpuFenceHandle to another
// process without using a local GL context. Intended for use in a Renderer
// that wants to forward a received GpuFenceHandle to the GPU process.
class GFX_EXPORT GpuFencePassthrough : public GpuFence {
 public:
  // Constructor takes ownership of the source handle's resources.
  explicit GpuFencePassthrough(const GpuFenceHandle& handle);
  ~GpuFencePassthrough() override;

  GpuFenceHandle GetGpuFenceHandle() override;

 private:
  gfx::GpuFenceHandleType type_;
#if defined(OS_POSIX)
  base::ScopedFD owned_fd_;
#endif

  DISALLOW_COPY_AND_ASSIGN(GpuFencePassthrough);
};

}  // namespace gfx

#endif  // UI_GL_GPU_FENCE_PASSTHROUGH_H_
