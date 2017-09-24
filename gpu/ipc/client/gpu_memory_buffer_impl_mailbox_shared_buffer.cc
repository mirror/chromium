// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/client/gpu_memory_buffer_impl_mailbox_shared_buffer.h"
#include "gpu/ipc/common/gpu_memory_buffer_support.h"

#include "base/logging.h"

namespace gpu {

GpuMemoryBufferImplMailboxSharedBuffer::GpuMemoryBufferImplMailboxSharedBuffer(
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    const DestructionCallback& callback,
    const gpu::Mailbox& mailbox)
    : GpuMemoryBufferImpl(id, size, format, callback) {
  // TODO(klausw): presumably there's a cleaner way to do this?
  mailbox_ = base::WrapUnique(new gpu::Mailbox());
  *mailbox_ = mailbox;
  // DVLOG(3) << __FUNCTION__ << ";;; mailbox=" << mailbox.DebugString() << "
  // *mailbox_=" << mailbox_->DebugString();
}

GpuMemoryBufferImplMailboxSharedBuffer::
    ~GpuMemoryBufferImplMailboxSharedBuffer() {}

// static
std::unique_ptr<GpuMemoryBufferImplMailboxSharedBuffer>
GpuMemoryBufferImplMailboxSharedBuffer::CreateFromHandle(
    const gfx::GpuMemoryBufferHandle& handle,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    const DestructionCallback& callback) {
  // DVLOG(3) << __FUNCTION__ << ";;; handle_mailbox=" <<
  // handle.MailboxSharedBufferString();
  gpu::Mailbox mailbox;
  std::copy(std::begin(handle.mailbox_shared_buffer),
            std::end(handle.mailbox_shared_buffer), std::begin(mailbox.name));
  return base::WrapUnique(new GpuMemoryBufferImplMailboxSharedBuffer(
      handle.id, size, format, callback, mailbox));
}

// static
bool GpuMemoryBufferImplMailboxSharedBuffer::IsConfigurationSupported(
    gfx::BufferFormat format,
    gfx::BufferUsage usage) {
  return gpu::IsNativeGpuMemoryBufferConfigurationSupported(format, usage);
}

bool GpuMemoryBufferImplMailboxSharedBuffer::Map() {
  return false;
}

void* GpuMemoryBufferImplMailboxSharedBuffer::memory(size_t plane) {
  return nullptr;
}

void GpuMemoryBufferImplMailboxSharedBuffer::Unmap() {}

int GpuMemoryBufferImplMailboxSharedBuffer::stride(size_t plane) const {
  return 0;
}

gfx::GpuMemoryBufferHandle GpuMemoryBufferImplMailboxSharedBuffer::GetHandle()
    const {
  return GetHandleForMailbox(*mailbox_, id_);
}

// static
gfx::GpuMemoryBufferHandle
GpuMemoryBufferImplMailboxSharedBuffer::GetHandleForMailbox(
    const Mailbox& mailbox,
    gfx::GpuMemoryBufferId id) {
  gfx::GpuMemoryBufferHandle handle;
  handle.type = gfx::MAILBOX_SHARED_BUFFER;
  handle.id = id;
  std::copy(std::begin(mailbox.name), std::end(mailbox.name),
            std::begin(handle.mailbox_shared_buffer));
  return handle;
}

}  // namespace gpu
