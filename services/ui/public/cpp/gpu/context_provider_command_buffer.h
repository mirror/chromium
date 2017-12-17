// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_PUBLIC_CPP_GPU_CONTEXT_PROVIDER_COMMAND_BUFFER_H_
#define SERVICES_UI_PUBLIC_CPP_GPU_CONTEXT_PROVIDER_COMMAND_BUFFER_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/compiler_specific.h"
#include "base/observer_list.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_checker.h"
#include "base/trace_event/memory_dump_provider.h"
#include "components/viz/common/gpu/context_provider.h"
#include "gpu/command_buffer/client/shared_memory_limits.h"
#include "gpu/command_buffer/common/gles2_cmd_utils.h"
#include "gpu/command_buffer/common/scheduling_priority.h"
#include "gpu/ipc/common/surface_handle.h"
#include "services/ui/public/cpp/gpu/command_buffer_metrics.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer_base.h"
#include "ui/gl/gpu_preference.h"
#include "url/gurl.h"

namespace gpu {
class CommandBufferProxyImpl;
class GpuChannelHost;
struct GpuFeatureInfo;
}

namespace ui {

// Implementation of viz::GLContextProvider that provides a GL implementation
// over command buffer to the GPU process.
class ContextProviderCommandBuffer : public viz::GLContextProvider,
                                     public ContextProviderCommandBufferBase {
 public:
  ContextProviderCommandBuffer(
      scoped_refptr<gpu::GpuChannelHost> channel,
      int32_t stream_id,
      gpu::SchedulingPriority stream_priority,
      gpu::SurfaceHandle surface_handle,
      const GURL& active_url,
      bool automatic_flushes,
      bool support_locking,
      const gpu::SharedMemoryLimits& memory_limits,
      const gpu::gles2::ContextCreationAttribHelper& attributes,
      ContextProviderCommandBufferBase* shared_context_provider,
      command_buffer_metrics::ContextType type);

  // viz::CommonContextProvider implementation.
  gpu::ContextResult BindToCurrentThread() override;
  gpu::ContextSupport* ContextSupport() override;
  class GrContext* GrContext() override;
  viz::ContextCacheController* CacheController() override;
  void InvalidateGrContext(uint32_t state) override;
  base::Lock* GetLock() override;
  const gpu::Capabilities& ContextCapabilities() const override;
  const gpu::GpuFeatureInfo& GetGpuFeatureInfo() const override;
  void AddObserver(viz::ContextLostObserver* obs) override;
  void RemoveObserver(viz::ContextLostObserver* obs) override;

  // viz::GLContextProvider implementation.
  gpu::gles2::GLES2Interface* ContextGL() override;

 protected:
  ~ContextProviderCommandBuffer() override;
};

class RasterContextProviderCommandBuffer
    : public viz::RasterContextProvider,
      public ui::ContextProviderCommandBufferBase {
 public:
  RasterContextProviderCommandBuffer(
      scoped_refptr<gpu::GpuChannelHost> channel,
      int32_t stream_id,
      gpu::SchedulingPriority stream_priority,
      gpu::SurfaceHandle surface_handle,
      const GURL& active_url,
      bool automatic_flushes,
      bool support_locking,
      const gpu::SharedMemoryLimits& memory_limits,
      const gpu::gles2::ContextCreationAttribHelper& attributes,
      ContextProviderCommandBufferBase* shared_context_provider,
      command_buffer_metrics::ContextType type);

  // viz::CommonContextProvider implementation.
  gpu::ContextResult BindToCurrentThread() override;
  gpu::ContextSupport* ContextSupport() override;
  class GrContext* GrContext() override;
  viz::ContextCacheController* CacheController() override;
  void InvalidateGrContext(uint32_t state) override;
  base::Lock* GetLock() override;
  const gpu::Capabilities& ContextCapabilities() const override;
  const gpu::GpuFeatureInfo& GetGpuFeatureInfo() const override;
  void AddObserver(viz::ContextLostObserver* obs) override;
  void RemoveObserver(viz::ContextLostObserver* obs) override;

  // viz::RasterContextProvider implementation.
  gpu::raster::RasterInterface* RasterInterface() override;

 protected:
  ~RasterContextProviderCommandBuffer() override;
};

}  // namespace ui

#endif  // SERVICES_UI_PUBLIC_CPP_GPU_CONTEXT_PROVIDER_COMMAND_BUFFER_H_
