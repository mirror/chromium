// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/client/gpu_memory_buffer_impl_android_hardware_buffer.h"

#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "gpu/ipc/client/gpu_memory_buffer_impl_android_hardware_buffer.h"
#include "gpu/ipc/common/gpu_memory_buffer_support.h"
#include "ui/gfx/buffer_format_util.h"
//#include "ui/gfx/client_native_pixmap_factory.h"
#include "ui/gfx/android/hardware_buffer_handle.h"

namespace gpu {

GpuMemoryBufferImplAndroidHardwareBuffer::
    GpuMemoryBufferImplAndroidHardwareBuffer(
        gfx::GpuMemoryBufferId id,
        const gfx::Size& size,
        gfx::BufferFormat format,
        const DestructionCallback& callback,
        gfx::RawAHardwareBuffer* ahb)
    : GpuMemoryBufferImpl(id, size, format, callback), hardware_buffer_(ahb) {
  // LOG(INFO) << __FUNCTION__ << ";;;";
}

GpuMemoryBufferImplAndroidHardwareBuffer::
    ~GpuMemoryBufferImplAndroidHardwareBuffer() {}

// static
std::unique_ptr<GpuMemoryBufferImplAndroidHardwareBuffer>
GpuMemoryBufferImplAndroidHardwareBuffer::CreateFromHandle(
    const gfx::GpuMemoryBufferHandle& handle,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    const DestructionCallback& callback) {
  // LOG(INFO) << __FUNCTION__ << ";;;";

  gfx::RawAHardwareBuffer* ahb =
      handle.hardware_buffer_handle.scoped_ahardwarebuffer.get();
  return base::WrapUnique(new GpuMemoryBufferImplAndroidHardwareBuffer(
      handle.id, size, format, callback, ahb));
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
  // LOG(INFO) << __FUNCTION__ << ";;;";
  if (!hardware_buffer_) {
    LOG(ERROR) << __FUNCTION__ << ";;; NO HARDWARE BUFFER";
  }
  return GetHandleFromRawAHardwareBuffer(hardware_buffer_);
}

// static
gfx::GpuMemoryBufferHandle
GpuMemoryBufferImplAndroidHardwareBuffer::GetHandleFromRawAHardwareBuffer(
    gfx::RawAHardwareBuffer* ahardwarebuffer) {
  // LOG(INFO) << __FUNCTION__ << ";;;";
  gfx::GpuMemoryBufferHandle handle;

  handle.type = gfx::EMPTY_BUFFER;  // Overwritten below on success, this is
                                    // only used on error.

  // From here, commit to returning the buffer successfully.
  handle.type = gfx::ANDROID_HARDWARE_BUFFER;
  handle.hardware_buffer_handle.scoped_ahardwarebuffer.reset(ahardwarebuffer);
  // handle.id = 0;

  return handle;
}

}  // namespace gpu
