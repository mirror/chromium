// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_COMMON_MUS_GPU_MEMORY_BUFFER_MANAGER_H_
#define SERVICES_UI_COMMON_MUS_GPU_MEMORY_BUFFER_MANAGER_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "services/ui/gpu/interfaces/gpu_service_internal.mojom.h"

namespace ui {

// This GpuMemoryBufferManager is for establishing a GpuChannelHost used by
// mus locally.
class MusGpuMemoryBufferManager : public gpu::GpuMemoryBufferManager,
                                  public base::ThreadChecker {
 public:
  MusGpuMemoryBufferManager(mojom::GpuServiceInternal* gpu_service,
                            int client_id);
  ~MusGpuMemoryBufferManager() override;

  // Overridden from gpu::GpuMemoryBufferManager:
  std::unique_ptr<gfx::GpuMemoryBuffer> CreateGpuMemoryBuffer(
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      gpu::SurfaceHandle surface_handle) override;
  std::unique_ptr<gfx::GpuMemoryBuffer> CreateGpuMemoryBufferFromHandle(
      const gfx::GpuMemoryBufferHandle& handle,
      const gfx::Size& size,
      gfx::BufferFormat format) override;
  void SetDestructionSyncToken(gfx::GpuMemoryBuffer* buffer,
                               const gpu::SyncToken& sync_token) override;

 private:
  void DestroyGpuMemoryBuffer(gfx::GpuMemoryBufferId id,
                              int client_id,
                              bool is_native,
                              const gpu::SyncToken& sync_token);

  mojom::GpuServiceInternal* gpu_service_;

  const int client_id_;
  int next_gpu_memory_id_ = 1;
  base::WeakPtrFactory<MusGpuMemoryBufferManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MusGpuMemoryBufferManager);
};

}  // namespace ui

#endif  // SERVICES_UI_COMMON_MUS_GPU_MEMORY_BUFFER_MANAGER_H_
