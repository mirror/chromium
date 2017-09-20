// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "gpu/ipc/client/gpu_memory_buffer_impl_dxgi.h"
#include "base/memory/ptr_util.h"
#include "ui/gfx/buffer_format_util.h"

namespace gpu {

GpuMemoryBufferImplDxgi::~GpuMemoryBufferImplDxgi() {}

std::unique_ptr<GpuMemoryBufferImplDxgi>
GpuMemoryBufferImplDxgi::CreateFromHandle(
    const gfx::GpuMemoryBufferHandle& handle,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    const DestructionCallback& callback) {
  return base::WrapUnique(new GpuMemoryBufferImplDxgi(
      handle.id, size, format, callback, handle.dxgi_handle.get_handle()));
}

bool GpuMemoryBufferImplDxgi::Map() {
  return false;  // The current implementation doesn't support mapping.
}

void* GpuMemoryBufferImplDxgi::memory(size_t plane) {
  return nullptr;  // The current implementation doesn't support mapping.
}

void GpuMemoryBufferImplDxgi::Unmap() {}

int GpuMemoryBufferImplDxgi::stride(size_t plane) const {
  return gfx::RowSizeForBufferFormat(size_.width(), format_, plane);
}

gfx::GpuMemoryBufferHandle GpuMemoryBufferImplDxgi::GetHandle() const {
  gfx::GpuMemoryBufferHandle handle;
  handle.type = gfx::DXGI_SHARED_HANDLE;
  handle.id = id_;
  handle.offset = 0;
  handle.stride = stride(0);
  handle.dxgi_handle.set_handle(handle_.Get());
  return handle;
}

GpuMemoryBufferImplDxgi::GpuMemoryBufferImplDxgi(
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    const DestructionCallback& callback,
    HANDLE dxgi_handle)
    : GpuMemoryBufferImpl(id, size, format, callback), handle_(dxgi_handle) {}

}  // namespace gpu