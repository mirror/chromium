// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/client/gpu_memory_buffer_impl.h"

#include "base/logging.h"
#include "build/build_config.h"
#include "gpu/ipc/client/gpu_memory_buffer_impl_shared_memory.h"

#if defined(OS_MACOSX)
#include "gpu/ipc/client/gpu_memory_buffer_impl_io_surface.h"
#endif

#if defined(OS_LINUX)
#include "gpu/ipc/client/gpu_memory_buffer_impl_native_pixmap.h"
#endif

#if defined(OS_WIN)
#include "gpu/ipc/client/gpu_memory_buffer_impl_dxgi.h"
#endif

#if defined(OS_ANDROID)
#include "gpu/ipc/client/gpu_memory_buffer_impl_android_hardware_buffer.h"
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2extchromium.h>

namespace gpu {

GpuMemoryBufferImpl::GpuMemoryBufferImpl(gfx::GpuMemoryBufferId id,
                                         const gfx::Size& size,
                                         gfx::BufferFormat format,
                                         const DestructionCallback& callback)
    : id_(id),
      size_(size),
      format_(format),
      callback_(callback),
      mapped_(false) {}

GpuMemoryBufferImpl::~GpuMemoryBufferImpl() {
  DCHECK(!mapped_);
  if (!callback_.is_null())
    callback_.Run(destruction_sync_token_);
}

// static
std::unique_ptr<GpuMemoryBufferImpl> GpuMemoryBufferImpl::CreateFromHandle(
    const gfx::GpuMemoryBufferHandle& handle,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    const DestructionCallback& callback) {
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
          handle, size, format, usage, callback);
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

gfx::Size GpuMemoryBufferImpl::GetSize() const {
  return size_;
}

gfx::BufferFormat GpuMemoryBufferImpl::GetFormat() const {
  return format_;
}

gfx::GpuMemoryBufferId GpuMemoryBufferImpl::GetId() const {
  return id_;
}

ClientBuffer GpuMemoryBufferImpl::AsClientBuffer() {
  return reinterpret_cast<ClientBuffer>(this);
}

uint32_t GpuMemoryBufferImpl::GetInternalformat(bool supports_bgra_ext) const {
  switch (format_) {
    case gfx::BufferFormat::ATC:
      return GL_ATC_RGB_AMD;
    case gfx::BufferFormat::ATCIA:
      return GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD;
    case gfx::BufferFormat::DXT1:
      return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
    case gfx::BufferFormat::DXT5:
      return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    case gfx::BufferFormat::ETC1:
      return GL_ETC1_RGB8_OES;
    case gfx::BufferFormat::R_8:
      return GL_RED_EXT;
    case gfx::BufferFormat::R_16:
      return GL_R16_EXT;
    case gfx::BufferFormat::RG_88:
      return GL_RG_EXT;
    case gfx::BufferFormat::BGR_565:
    case gfx::BufferFormat::RGBX_8888:
    case gfx::BufferFormat::BGRX_8888:
    case gfx::BufferFormat::BGRX_1010102:
      return GL_RGB;
    case gfx::BufferFormat::RGBA_4444:
    case gfx::BufferFormat::RGBA_8888:
    case gfx::BufferFormat::RGBA_F16:
    case gfx::BufferFormat::BGRA_8888:
      return GL_RGBA;
    case gfx::BufferFormat::YVU_420:
      return GL_RGB_YCRCB_420_CHROMIUM;
    case gfx::BufferFormat::YUV_420_BIPLANAR:
      return GL_RGB_YCBCR_420V_CHROMIUM;
    case gfx::BufferFormat::UYVY_422:
      return GL_RGB_YCBCR_422_CHROMIUM;
    default:
      NOTREACHED();
      return GL_NONE;
  }
}


}  // namespace gpu
