// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_CLIENT_GPU_MEMORY_BUFFER_IMPL_DXGI_H_
#define GPU_IPC_CLIENT_GPU_MEMORY_BUFFER_IMPL_DXGI_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/win/scoped_handle.h"
#include "gpu/gpu_export.h"
#include "gpu/ipc/client/gpu_memory_buffer_impl.h"
#include "ui/gfx/color_space.h"

namespace gpu {

// Implementation of GPU memory buffer based on dxgi textures.
class GPU_EXPORT GpuMemoryBufferImplDxgi : public GpuMemoryBufferImpl {
 public:
  ~GpuMemoryBufferImplDxgi() override;

  static std::unique_ptr<GpuMemoryBufferImplDxgi> CreateFromHandle(
      const gfx::GpuMemoryBufferHandle& handle,
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      const DestructionCallback& callback);

  bool Map() override;
  void* memory(size_t plane) override;
  void Unmap() override;
  int stride(size_t plane) const override;
  gfx::GpuMemoryBufferHandle GetHandle() const override;

 private:
  GpuMemoryBufferImplDxgi(gfx::GpuMemoryBufferId id,
                          const gfx::Size& size,
                          gfx::BufferFormat format,
                          const DestructionCallback& callback,
                          HANDLE dxgi_handle);

  base::win::ScopedHandle handle_;

  DISALLOW_COPY_AND_ASSIGN(GpuMemoryBufferImplDxgi);
};

}  // namespace gpu

#endif  // GPU_IPC_CLIENT_GPU_MEMORY_BUFFER_IMPL_DXGI_H_
