// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/client/gpu_memory_buffer_impl_factory.h"

#include "base/logging.h"
#include "build/build_config.h"
#include "gpu/ipc/client/gpu_memory_buffer_impl_shared_memory.h"

#if defined(OS_MACOSX)
#include "gpu/ipc/client/gpu_memory_buffer_impl_io_surface.h"
#endif

#if defined(OS_LINUX)
#include "gpu/ipc/client/gpu_memory_buffer_impl_native_pixmap.h"
#include "ui/gfx/linux/client_native_pixmap_factory_dmabuf.h"
#endif

#if defined(USE_OZONE)
#include "ui/ozone/public/client_native_pixmap_factory_ozone.h"
#endif

#if defined(OS_WIN)
#include "gpu/ipc/client/gpu_memory_buffer_impl_dxgi.h"
#endif

#if defined(OS_ANDROID)
#include "base/android/android_hardware_buffer_compat.h"
#include "gpu/ipc/client/gpu_memory_buffer_impl_android_hardware_buffer.h"
#endif

namespace gpu {

GpuMemoryBufferImplFactory::GpuMemoryBufferImplFactory() {}

#if defined(OS_LINUX)
GpuMemoryBufferImplFactory::GpuMemoryBufferImplFactory(
    std::unique_ptr<gfx::ClientNativePixmapFactory> pixmap_factory)
    : GpuMemoryBufferSupport(std::move(pixmap_factory)) {}
#endif

GpuMemoryBufferImplFactory::~GpuMemoryBufferImplFactory() {}

bool GpuMemoryBufferImplFactory::IsConfigurationSupported(
    gfx::GpuMemoryBufferType type,
    gfx::BufferFormat format,
    gfx::BufferUsage usage) {
  if (type == GetNativeGpuMemoryBufferType())
    return IsNativeGpuMemoryBufferConfigurationSupported(format, usage);

  if (type == gfx::SHARED_MEMORY_BUFFER) {
    return GpuMemoryBufferImplSharedMemory::IsConfigurationSupported(format,
                                                                     usage);
  }

  return false;
}

std::unique_ptr<GpuMemoryBufferImpl>
GpuMemoryBufferImplFactory::CreateFromHandle(
    const gfx::GpuMemoryBufferHandle& handle,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    const GpuMemoryBufferImpl::DestructionCallback& callback) {
  switch (handle.type) {
    case gfx::SHARED_MEMORY_BUFFER:
      return GpuMemoryBufferImplSharedMemory::CreateFromHandle(
          handle, size, format, usage, callback);
#if defined(OS_MACOSX)
    case gfx::IO_SURFACE_BUFFER:
      return GpuMemoryBufferImplIOSurface::CreateFromHandle(
          handle, size, format, usage, callback);
#endif
#if defined(OS_LINUX)
    case gfx::NATIVE_PIXMAP:
      return GpuMemoryBufferImplNativePixmap::CreateFromHandle(
          pixmap_factory(), handle, size, format, usage, callback);
#endif
#if defined(OS_WIN)
    case gfx::DXGI_SHARED_HANDLE:
      return GpuMemoryBufferImplDXGI::CreateFromHandle(handle, size, format,
                                                       usage, callback);
#endif
#if defined(OS_ANDROID)
    case gfx::ANDROID_HARDWARE_BUFFER:
      return GpuMemoryBufferImplAndroidHardwareBuffer::CreateFromHandle(
          handle, size, format, usage, callback);
#endif
    default:
      NOTREACHED();
      return nullptr;
  }
}

std::unique_ptr<GpuMemoryBufferImpl> GpuMemoryBufferImplFactory::Create(
    gfx::GpuMemoryBufferType type,
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    const GpuMemoryBufferImpl::DestructionCallback& callback) {
  switch (type) {
    case gfx::SHARED_MEMORY_BUFFER:
      return GpuMemoryBufferImplSharedMemory::Create(id, size, format, usage,
                                                     callback);
#if defined(OS_ANDROID)
    case gfx::ANDROID_HARDWARE_BUFFER:
      return GpuMemoryBufferImplAndroidHardwareBuffer::Create(id, size, format,
                                                              usage, callback);
#endif
    default:
      NOTREACHED();
      return nullptr;
  }
}

}  // namespace gpu
