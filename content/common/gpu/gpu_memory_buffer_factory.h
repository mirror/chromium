// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_GPU_GPU_MEMORY_BUFFER_FACTORY_H_
#define CONTENT_COMMON_GPU_GPU_MEMORY_BUFFER_FACTORY_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "content/common/content_export.h"
#include "gpu/ipc/common/surface_handle.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/gpu_memory_buffer.h"

namespace gpu {
class ImageFactory;
}

namespace content {

class CONTENT_EXPORT GpuMemoryBufferFactory {
 public:
  virtual ~GpuMemoryBufferFactory() {}

  // Creates a new factory instance for native GPU memory buffers.
  static scoped_ptr<GpuMemoryBufferFactory> CreateNativeType();

  // Creates a new GPU memory buffer instance. A valid handle is returned on
  // success. It can be called on any thread.
  virtual gfx::GpuMemoryBufferHandle CreateGpuMemoryBuffer(
      gfx::GpuMemoryBufferId id,
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      int client_id,
      gpu::SurfaceHandle surface_handle) = 0;

  // Creates a new GPU memory buffer instance from an existing handle. A valid
  // handle is returned on success. It can be called on any thread.
  virtual gfx::GpuMemoryBufferHandle CreateGpuMemoryBufferFromHandle(
      const gfx::GpuMemoryBufferHandle& handle,
      gfx::GpuMemoryBufferId id,
      const gfx::Size& size,
      gfx::BufferFormat format,
      int client_id) = 0;

  // Destroys GPU memory buffer identified by |id|.
  // It can be called on any thread.
  virtual void DestroyGpuMemoryBuffer(gfx::GpuMemoryBufferId id,
                                      int client_id) = 0;

  // Type-checking downcast routine.
  virtual gpu::ImageFactory* AsImageFactory() = 0;

 protected:
  GpuMemoryBufferFactory() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(GpuMemoryBufferFactory);
};

}  // namespace content

#endif  // CONTENT_COMMON_GPU_GPU_MEMORY_BUFFER_FACTORY_H_
