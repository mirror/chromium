// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_CLIENT_GPU_MEMORY_BUFFER_IMPL_MAILBOX_SHARED_BUFFER_H_
#define GPU_IPC_CLIENT_GPU_MEMORY_BUFFER_IMPL_MAILBOX_SHARED_BUFFER_H_

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "gpu/gpu_export.h"
#include "gpu/ipc/client/gpu_memory_buffer_impl.h"

namespace gpu {

struct Mailbox;

class GPU_EXPORT GpuMemoryBufferImplMailboxSharedBuffer : public GpuMemoryBufferImpl {
 public:
  ~GpuMemoryBufferImplMailboxSharedBuffer() override;

  static std::unique_ptr<GpuMemoryBufferImplMailboxSharedBuffer> CreateFromHandle(
      const gfx::GpuMemoryBufferHandle& handle,
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      const DestructionCallback& callback);

  static bool IsConfigurationSupported(gfx::BufferFormat format,
                                       gfx::BufferUsage usage);

#if 0
  static base::Closure AllocateForTesting(const gfx::Size& size,
                                          gfx::BufferFormat format,
                                          gfx::BufferUsage usage,
                                          gfx::GpuMemoryBufferHandle* handle);
#endif

  // Overridden from gfx::GpuMemoryBuffer:
  bool Map() override;
  void* memory(size_t plane) override;
  void Unmap() override;
  int stride(size_t plane) const override;
  gfx::GpuMemoryBufferHandle GetHandle() const override;

  static gfx::GpuMemoryBufferHandle GetHandleForMailbox(const Mailbox&, gfx::GpuMemoryBufferId);

 private:
  GpuMemoryBufferImplMailboxSharedBuffer(
      gfx::GpuMemoryBufferId id,
      const gfx::Size& size,
      gfx::BufferFormat format,
      const DestructionCallback& callback,
      const gpu::Mailbox& mailbox);

  std::unique_ptr<gpu::Mailbox> mailbox_;

  DISALLOW_COPY_AND_ASSIGN(GpuMemoryBufferImplMailboxSharedBuffer);
};

}  // namespace gpu

#endif  // GPU_IPC_CLIENT_GPU_MEMORY_BUFFER_IMPL_MAILBOX_SHARED_BUFFER_H_
