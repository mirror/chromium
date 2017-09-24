// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_CLIENT_GPU_MEMORY_BUFFER_IMPL_ANDROID_HARDWARE_BUFFER_H_
#define GPU_IPC_CLIENT_GPU_MEMORY_BUFFER_IMPL_ANDROID_HARDWARE_BUFFER_H_

#include <stddef.h>

#include <memory>

#include "base/macros.h"
#include "gpu/gpu_export.h"
#include "gpu/ipc/client/gpu_memory_buffer_impl.h"
#include "ui/gfx/android/hardware_buffer_handle.h"

namespace gfx {
class ClientAndroidHardwareBuffer;
}

namespace gpu {

// Implementation of GPU memory buffer based on Android AHardwareBuffer.
class GPU_EXPORT GpuMemoryBufferImplAndroidHardwareBuffer
    : public GpuMemoryBufferImpl {
 public:
  ~GpuMemoryBufferImplAndroidHardwareBuffer() override;

  static std::unique_ptr<GpuMemoryBufferImplAndroidHardwareBuffer>
  CreateFromHandle(const gfx::GpuMemoryBufferHandle& handle,
                   const gfx::Size& size,
                   gfx::BufferFormat format,
                   gfx::BufferUsage usage,
                   const DestructionCallback& callback);

  static bool IsConfigurationSupported(gfx::BufferFormat format,
                                       gfx::BufferUsage usage);

  static base::Closure AllocateForTesting(const gfx::Size& size,
                                          gfx::BufferFormat format,
                                          gfx::BufferUsage usage,
                                          gfx::GpuMemoryBufferHandle* handle);

  // Overridden from gfx::GpuMemoryBuffer:
  bool Map() override;
  void* memory(size_t plane) override;
  void Unmap() override;
  int stride(size_t plane) const override;
  gfx::GpuMemoryBufferHandle GetHandle() const override;
  gfx::RawAHardwareBuffer* GetRawAHardwareBuffer() { return hardware_buffer_; }

  static gfx::GpuMemoryBufferHandle GetHandleFromRawAHardwareBuffer(
      gfx::RawAHardwareBuffer* ahardwarebuffer);

 private:
  GpuMemoryBufferImplAndroidHardwareBuffer(gfx::GpuMemoryBufferId id,
                                           const gfx::Size& size,
                                           gfx::BufferFormat format,
                                           const DestructionCallback& callback,
                                           gfx::RawAHardwareBuffer* raw_buffer);

  gfx::RawAHardwareBuffer* hardware_buffer_ = nullptr;
  base::ScopedFD tmp_socket_fd_for_renderer_;

  DISALLOW_COPY_AND_ASSIGN(GpuMemoryBufferImplAndroidHardwareBuffer);
};

}  // namespace gpu

#endif  // GPU_IPC_CLIENT_GPU_MEMORY_BUFFER_IMPL_ANDROID_HARDWARE_BUFFER_H_
