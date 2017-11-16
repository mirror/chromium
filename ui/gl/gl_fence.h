// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_FENCE_H_
#define UI_GL_GL_FENCE_H_

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "ui/gfx/gpu_fence_handle.h"
#include "ui/gl/gl_export.h"

namespace gl {

class GL_EXPORT GLFence {
 public:
  GLFence();
  virtual ~GLFence();

  static bool IsSupported();
  static GLFence* Create();

  virtual bool HasCompleted() = 0;
  virtual void ClientWait() = 0;

  // Will block the server if supported, but might fall back to blocking the
  // client.
  virtual void ServerWait() = 0;

  // Returns true if re-setting state is supported.
  virtual bool ResetSupported();

  // Resets the fence to the original state.
  virtual void ResetState();

  // Loses the reference to the fence. Useful if the context is lost.
  virtual void Invalidate();

  // GpuFenceHandle integration.
  static bool IsGpuFenceSupported();

  // Consumes the GpuFenceHandle to create a paired local GL fence.
  static std::unique_ptr<GLFence> CreateFromGpuFenceHandle(
      const gfx::GpuFenceHandle& handle);

  // Create a new GLFence that can be used with GetGpuFenceHandle.
  static std::unique_ptr<GLFence> CreateForGpuFence();

  // Extracts a GpuFenceHandle. This handle must be used to construct a
  // GpuFence (or passed to IPC directly) to avoid leaking file descriptors or
  // other resources.
  virtual gfx::GpuFenceHandle GetGpuFenceHandle();

 private:
  DISALLOW_COPY_AND_ASSIGN(GLFence);
};

}  // namespace gl

#endif  // UI_GL_GL_FENCE_H_
