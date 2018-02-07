// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_CLIENT_GPU_MEMORY_BUFFER_IMPL_FACTORY_H_
#define GPU_IPC_CLIENT_GPU_MEMORY_BUFFER_IMPL_FACTORY_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "gpu/gpu_export.h"
#include "gpu/ipc/client/gpu_memory_buffer_impl.h"
#include "gpu/ipc/common/gpu_memory_buffer_support.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/gpu_memory_buffer.h"

namespace gpu {

// Provides a common factory for GPU memory buffer implementations.
class GPU_EXPORT GpuMemoryBufferImplFactory : public GpuMemoryBufferSupport {
 public:
  GpuMemoryBufferImplFactory();
#if defined(OS_LINUX)
  GpuMemoryBufferImplFactory(
      std::unique_ptr<gfx::ClientNativePixmapFactory> pixmap_factory);
#endif
  ~GpuMemoryBufferImplFactory();

  // Returns whether the provided buffer format is supported.
  bool IsConfigurationSupported(gfx::GpuMemoryBufferType type,
                                gfx::BufferFormat format,
                                gfx::BufferUsage usage);

  // Creates an instance from the given |handle|. |size| and |format|
  // should match what was used to allocate the |handle|. |callback|, if
  // non-null, is called when instance is deleted, which is not necessarily on
  // the same thread as this function was called on and instance was created on.
  std::unique_ptr<GpuMemoryBufferImpl> CreateFromHandle(
      const gfx::GpuMemoryBufferHandle& handle,
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      const GpuMemoryBufferImpl::DestructionCallback& callback);

  // Creates a new instance with the given |type| and |id|. |callback|, if
  // non-null, is called when instance is deleted, which is not necessarily on
  // the same thread as this function was called on and instance was created on.
  std::unique_ptr<GpuMemoryBufferImpl> Create(
      gfx::GpuMemoryBufferType type,
      gfx::GpuMemoryBufferId id,
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      const GpuMemoryBufferImpl::DestructionCallback& callback);

 private:
  DISALLOW_COPY_AND_ASSIGN(GpuMemoryBufferImplFactory);
};

}  // namespace gpu

#endif  // GPU_IPC_CLIENT_GPU_MEMORY_BUFFER_IMPL_FACTORY_H_
