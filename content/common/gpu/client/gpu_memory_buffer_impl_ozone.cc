// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/gpu/client/gpu_memory_buffer_impl.h"

#include "content/common/gpu/client/gpu_memory_buffer_impl_ozone_native_buffer.h"
#include "content/common/gpu/client/gpu_memory_buffer_impl_shared_memory.h"

namespace content {

// static
void GpuMemoryBufferImpl::Create(const gfx::Size& size,
                                 unsigned internalformat,
                                 unsigned usage,
                                 int client_id,
                                 const CreationCallback& callback) {
  if (GpuMemoryBufferImplOzoneNativeBuffer::IsConfigurationSupported(
          internalformat, usage)) {
    GpuMemoryBufferImplOzoneNativeBuffer::Create(
        size, internalformat, usage, client_id, callback);
    return;
  }

  if (GpuMemoryBufferImplSharedMemory::IsConfigurationSupported(
          size, internalformat, usage)) {
    GpuMemoryBufferImplSharedMemory::Create(
        size, internalformat, usage, callback);
    return;
  }

  callback.Run(scoped_ptr<GpuMemoryBufferImpl>());
}

// static
void GpuMemoryBufferImpl::AllocateForChildProcess(
    const gfx::Size& size,
    unsigned internalformat,
    unsigned usage,
    base::ProcessHandle child_process,
    int child_client_id,
    const AllocationCallback& callback) {
  if (GpuMemoryBufferImplOzoneNativeBuffer::IsConfigurationSupported(
          internalformat, usage)) {
    GpuMemoryBufferImplOzoneNativeBuffer::
        AllocateOzoneNativeBufferForChildProcess(
            size, internalformat, usage, child_client_id, callback);
    return;
  }
  if (GpuMemoryBufferImplSharedMemory::IsConfigurationSupported(
          size, internalformat, usage)) {
    GpuMemoryBufferImplSharedMemory::AllocateSharedMemoryForChildProcess(
        size, internalformat, child_process, callback);
    return;
  }

  callback.Run(gfx::GpuMemoryBufferHandle());
}

// static
void GpuMemoryBufferImpl::DeletedByChildProcess(
    gfx::GpuMemoryBufferType type,
    const gfx::GpuMemoryBufferId& id,
    base::ProcessHandle child_process) {
}

// static
scoped_ptr<GpuMemoryBufferImpl> GpuMemoryBufferImpl::CreateFromHandle(
    const gfx::GpuMemoryBufferHandle& handle,
    const gfx::Size& size,
    unsigned internalformat) {
  switch (handle.type) {
    case gfx::SHARED_MEMORY_BUFFER: {
      scoped_ptr<GpuMemoryBufferImplSharedMemory> buffer(
          new GpuMemoryBufferImplSharedMemory(size, internalformat));
      if (!buffer->InitializeFromHandle(handle))
        return scoped_ptr<GpuMemoryBufferImpl>();

      return buffer.PassAs<GpuMemoryBufferImpl>();
    }
    case gfx::OZONE_NATIVE_BUFFER: {
      scoped_ptr<GpuMemoryBufferImplOzoneNativeBuffer> buffer(
          new GpuMemoryBufferImplOzoneNativeBuffer(size, internalformat));
      if (!buffer->InitializeFromHandle(handle))
        return scoped_ptr<GpuMemoryBufferImpl>();

      return buffer.PassAs<GpuMemoryBufferImpl>();
    }
    default:
      return scoped_ptr<GpuMemoryBufferImpl>();
  }
}

}  // namespace content
