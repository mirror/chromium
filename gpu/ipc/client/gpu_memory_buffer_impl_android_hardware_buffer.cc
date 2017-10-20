// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/client/gpu_memory_buffer_impl_android_hardware_buffer.h"

#include <utility>

#include "base/android/android_hardware_buffer_compat.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "gpu/ipc/client/gpu_memory_buffer_impl_android_hardware_buffer.h"
#include "gpu/ipc/common/gpu_memory_buffer_support.h"
#include "ui/gfx/buffer_format_util.h"
#include "ui/gfx/geometry/size.h"

namespace gpu {

namespace {

AHardwareBuffer* CreateFromSpec(const gfx::Size& size,
                                gfx::BufferFormat format,
                                gfx::BufferUsage usage) {
  DVLOG(2) << __FUNCTION__;

  base::AndroidHardwareBufferCompat::EnsureFunctionsLoaded();

  AHardwareBuffer_Desc desc;
  // On create, all elements must be initialized, including setting the
  // "reserved for future use" (rfu) fields to zero.
  desc.width = size.width();
  desc.height = size.height();
  desc.layers = 1;  // number of images
  desc.stride = 0;  // ignored for AHardwareBuffer_allocate
  desc.rfu0 = 0;
  desc.rfu1 = 0;

  // TODO(klausw): support BGR_565? Native type's color order doesn't match:
  // AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM
  switch (format) {
    case gfx::BufferFormat::RGBA_8888:
      desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
      break;
    case gfx::BufferFormat::RGBX_8888:
      desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM;
      break;
    default:
      LOG(ERROR) << __FUNCTION__ << ": Unsupported gfx::BufferFormat: "
                 << static_cast<int>(format);
      return nullptr;
  }

  switch (usage) {
    case gfx::BufferUsage::SCANOUT:
      desc.usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
                   AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT;
      break;
    default:
      LOG(ERROR) << __FUNCTION__ << ": Unsupported gfx::BufferUsage: "
                 << static_cast<int>(usage);
      return nullptr;
  }

  AHardwareBuffer* ahb = nullptr;
  AHardwareBuffer_allocate(&desc, &ahb);
  if (!ahb) {
    LOG(ERROR) << __FUNCTION__ << ": Failed to allocate AHardwareBuffer";
    return nullptr;
  }

  return ahb;
}

}  // namespace

GpuMemoryBufferImplAndroidHardwareBuffer::
    GpuMemoryBufferImplAndroidHardwareBuffer(
        gfx::GpuMemoryBufferId id,
        const gfx::Size& size,
        gfx::BufferFormat format,
        const DestructionCallback& callback,
        const base::SharedMemoryHandle& buffer)
    : GpuMemoryBufferImpl(id, size, format, callback),
      shared_memory_(buffer, false) {
  DVLOG(2) << __FUNCTION__;
}

GpuMemoryBufferImplAndroidHardwareBuffer::
    ~GpuMemoryBufferImplAndroidHardwareBuffer() {}

// static
std::unique_ptr<GpuMemoryBufferImplAndroidHardwareBuffer>
GpuMemoryBufferImplAndroidHardwareBuffer::Create(
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    const DestructionCallback& callback) {
  size_t buffer_size = 0u;
  if (!gfx::BufferSizeForBufferFormatChecked(size, format, &buffer_size))
    return nullptr;

  struct AHardwareBuffer* buf = CreateFromSpec(size, format, usage);
  if (!buf) {
    return nullptr;
  }

  return base::WrapUnique(new GpuMemoryBufferImplAndroidHardwareBuffer(
      id, size, format, callback,
      base::SharedMemoryHandle(buf, 0, base::UnguessableToken::Create())));
}

// static
std::unique_ptr<GpuMemoryBufferImplAndroidHardwareBuffer>
GpuMemoryBufferImplAndroidHardwareBuffer::CreateFromHandle(
    const gfx::GpuMemoryBufferHandle& handle,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    const DestructionCallback& callback) {
  DVLOG(2) << __FUNCTION__;

  DCHECK(base::SharedMemory::IsHandleValid(handle.handle));
  return base::WrapUnique(new GpuMemoryBufferImplAndroidHardwareBuffer(
      handle.id, size, format, callback, handle.handle));
}

// static
bool GpuMemoryBufferImplAndroidHardwareBuffer::IsConfigurationSupported(
    gfx::BufferFormat format,
    gfx::BufferUsage usage) {
  return gpu::IsNativeGpuMemoryBufferConfigurationSupported(format, usage);
}

bool GpuMemoryBufferImplAndroidHardwareBuffer::Map() {
  return false;
}

void* GpuMemoryBufferImplAndroidHardwareBuffer::memory(size_t plane) {
  return nullptr;
}

void GpuMemoryBufferImplAndroidHardwareBuffer::Unmap() {}

int GpuMemoryBufferImplAndroidHardwareBuffer::stride(size_t plane) const {
  return 0;
}

gfx::GpuMemoryBufferHandle GpuMemoryBufferImplAndroidHardwareBuffer::GetHandle()
    const {
  DVLOG(2) << __FUNCTION__;
  gfx::GpuMemoryBufferHandle handle;
  handle.type = gfx::ANDROID_HARDWARE_BUFFER;
  handle.id = id_;
  handle.offset = 0;
  handle.stride = stride(0);
  handle.handle = shared_memory_.handle();

  return handle;
}

// static
gfx::GpuMemoryBufferHandle
GpuMemoryBufferImplAndroidHardwareBuffer::GetHandleFromRawAHardwareBuffer(
    AHardwareBuffer* ahardwarebuffer) {
  DVLOG(2) << __FUNCTION__;
  gfx::GpuMemoryBufferHandle handle;
  handle.type = gfx::ANDROID_HARDWARE_BUFFER;
  gfx::GpuMemoryBufferId kBufferId(1);
  handle.id = kBufferId;
  handle.handle = base::SharedMemoryHandle(ahardwarebuffer, 0,
                                           base::UnguessableToken::Create());
  return handle;
}

// static
base::Closure GpuMemoryBufferImplAndroidHardwareBuffer::AllocateForTesting(
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    gfx::GpuMemoryBufferHandle* handle) {
  gfx::GpuMemoryBufferId kBufferId(1);
  handle->type = gfx::ANDROID_HARDWARE_BUFFER;
  handle->id = kBufferId;
  struct AHardwareBuffer* buf = CreateFromSpec(size, format, usage);
  handle->handle =
      base::SharedMemoryHandle(buf, 0, base::UnguessableToken::Create());
  return base::Bind(&base::DoNothing);
}

}  // namespace gpu
