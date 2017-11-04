// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_GPU_FENCE_HANDLE_H_
#define UI_GFX_GPU_FENCE_HANDLE_H_

#include "base/file_descriptor_posix.h"
#include "base/memory/ptr_util.h"
#include "ui/gfx/gfx_export.h"

namespace gl {
class GLFence;
}

namespace gfx {

enum class GpuFenceHandleType {
  EMPTY,
  FILE_DESCRIPTOR,
  LAST = FILE_DESCRIPTOR
};

struct GFX_EXPORT GpuFenceHandle {
  // Creates a new EMPTY_FENCE handle. Use InsertFence to create a shareable
  // handle from it.
  GpuFenceHandle();

  GpuFenceHandle(const GpuFenceHandle& other);
  ~GpuFenceHandle();
  bool is_null() const { return type == GpuFenceHandleType::EMPTY; }

#if 0
  // Creates and inserts a new fence, and populates
  // the handle's data for IPC transport. Type must be
  // EMPTY_FENCE before this call.
  std::unique_ptr<gl::GLFence> InsertFence();

  // Inserts a new fence based on the handle's data.
  // This invalidates the handle and sets its type
  // to EMPTY_FENCE.
  std::unique_ptr<gl::GLFence> ExtractFence();
#endif

  GpuFenceHandleType type;
  base::FileDescriptor native_fd;
};

// Returns an instance of |handle| which can be sent over IPC. This duplicates
// the file-handles as appropriate, so that the IPC code take ownership of them,
// without invalidating |handle| itself.
GFX_EXPORT GpuFenceHandle
CloneHandleForIPC(const GpuFenceHandle& handle);

}  // namespace gfx

#endif  // UI_GFX_GPU_FENCE_HANDLE_H_
