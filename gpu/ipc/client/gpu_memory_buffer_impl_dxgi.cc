// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "gpu/ipc/client/gpu_memory_buffer_impl_dxgi.h"
#include "base/memory/ptr_util.h"
#include "ui/gfx/buffer_format_util.h"

namespace gpu {

GpuMemoryBufferImplDXGI::~GpuMemoryBufferImplDXGI() {}

std::unique_ptr<GpuMemoryBufferImplDXGI>
GpuMemoryBufferImplDXGI::CreateFromHandle(
    const gfx::GpuMemoryBufferHandle& handle,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    const DestructionCallback& callback) {
  DCHECK(base::SharedMemory::IsHandleValid(handle.handle));
  return base::WrapUnique(new GpuMemoryBufferImplDXGI(handle.id, size, format,
                                                      callback, handle.handle));
}

bool GpuMemoryBufferImplDXGI::Map() {
  return false;  // The current implementation doesn't support mapping.
}

void* GpuMemoryBufferImplDXGI::memory(size_t plane) {
  return nullptr;  // The current implementation doesn't support mapping.
}

void GpuMemoryBufferImplDXGI::Unmap() {}

int GpuMemoryBufferImplDXGI::stride(size_t plane) const {
  return gfx::RowSizeForBufferFormat(size_.width(), format_, plane);
}

gfx::GpuMemoryBufferHandle GpuMemoryBufferImplDXGI::GetHandle() const {
  gfx::GpuMemoryBufferHandle handle;
  handle.type = gfx::DXGI_SHARED_HANDLE;
  handle.id = id_;
  handle.offset = 0;
  handle.stride = stride(0);
  handle.handle = shared_memory_->handle();
  return handle;
}

GpuMemoryBufferImplDXGI::GpuMemoryBufferImplDXGI(
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    const DestructionCallback& callback,
    const base::SharedMemoryHandle& dxgi_handle)
    : GpuMemoryBufferImpl(id, size, format, callback),
      shared_memory_(base::MakeUnique<base::SharedMemory>(dxgi_handle, false)) {
}

}  // namespace gpu