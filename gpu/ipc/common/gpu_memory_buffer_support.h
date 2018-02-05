// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_COMMON_GPU_MEMORY_BUFFER_SUPPORT_H_
#define GPU_IPC_COMMON_GPU_MEMORY_BUFFER_SUPPORT_H_

#include "build/build_config.h"
#include "gpu/gpu_export.h"
#include "ui/gfx/buffer_types.h"
#include "ui/gfx/gpu_memory_buffer.h"

namespace gfx {
class ClientNativePixmapFactory;
}

namespace gpu {

class GPU_EXPORT GpuMemoryBufferSupport {
 public:
  GpuMemoryBufferSupport();
  ~GpuMemoryBufferSupport();

  // Returns the native GPU memory buffer factory type. Returns EMPTY_BUFFER
  // type if native buffers are not supported.
  GPU_EXPORT gfx::GpuMemoryBufferType GetNativeGpuMemoryBufferType();

  // Returns whether the provided buffer format is supported.
  GPU_EXPORT bool IsNativeGpuMemoryBufferConfigurationSupported(
      gfx::BufferFormat format,
      gfx::BufferUsage usage);

#if defined(OS_LINUX)
  gfx::ClientNativePixmapFactory* pixmap_factory() {
    return pixmap_factory_.get();
  }
#endif

 private:
#if defined(OS_LINUX)
  std::unique_ptr<gfx::ClientNativePixmapFactory> pixmap_factory_;
#endif

  DISALLOW_COPY_AND_ASSIGN(GpuMemoryBufferSupport);
};

}  // namespace gpu

#endif  // GPU_IPC_COMMON_GPU_MEMORY_BUFFER_SUPPORT_H_
