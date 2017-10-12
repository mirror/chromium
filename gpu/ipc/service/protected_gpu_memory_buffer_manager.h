// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_SERVICE_PROTECTED_GPU_MEMORY_BUFFER_MANAGER_H_
#define GPU_IPC_SERVICE_PROTECTED_GPU_MEMORY_BUFFER_MANAGER_H_

#include "base/macros.h"
#include "gpu/gpu_export.h"
#include "ui/gfx/gpu_memory_buffer.h"

namespace gpu {

// Provides a service to look up and access protected gpu memory buffers.
// A protected gpu memory buffer is a buffer that can be referred to by
// a different GpuMemoryBufferHandle, which does not provide access to the
// protected buffer's contents. Such handle can be shared with clients that
// are not allowed access to the protected buffer's contents. When passed to
// GetProtectedGpuMemoryBufferFor(), will result in returning another
// GpuMemoryBufferHandle that allows access to the protected buffer.
// For that reason, this interface should only be available to privileged users,
// which are allowed access to the contents of the protected buffer.
class GPU_EXPORT ProtectedGpuMemoryBufferManager {
 public:
  ProtectedGpuMemoryBufferManager() {}
  virtual ~ProtectedGpuMemoryBufferManager() {}

  // Return a GpuMemoryBufferHandle allowing access to the contents of the
  // protected buffer, for the given unprivileged |handle| to it, if one exists,
  // or a null handle otherwise.
  // The caller is responsible for closing the handle.
  virtual gfx::GpuMemoryBufferHandle GetProtectedGpuMemoryBufferFor(
      const gfx::GpuMemoryBufferHandle& handle) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(ProtectedGpuMemoryBufferManager);
};

}  // namespace gpu

#endif  // GPU_IPC_SERVICE_PROTECTED_GPU_MEMORY_BUFFER_MANAGER_H_
