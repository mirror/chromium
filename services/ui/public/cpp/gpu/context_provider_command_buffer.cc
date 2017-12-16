// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"

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
#include "gpu/ipc/client/gpu_channel_host.h"
#include "gpu/ipc/common/surface_handle.h"
#include "services/ui/public/cpp/gpu/command_buffer_metrics.h"
#include "ui/gl/gpu_preference.h"
#include "url/gurl.h"

namespace gpu {
class CommandBufferProxyImpl;
class GpuChannelHost;
struct GpuFeatureInfo;
class TransferBuffer;
namespace gles2 {
class GLES2CmdHelper;
class GLES2Implementation;
class GLES2TraceImplementation;
}  // namespace gles2
namespace raster {
class RasterImplementationGLES;
}
}  // namespace gpu

namespace skia_bindings {
class GrContextForGLES2Interface;
}

namespace ui {

ContextProviderCommandBuffer::ContextProviderCommandBuffer(
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
    command_buffer_metrics::ContextType type)
    : ContextProviderCommandBufferBase(channel,
                                       stream_id,
                                       stream_priority,
                                       surface_handle,
                                       active_url,
                                       automatic_flushes,
                                       support_locking,
                                       memory_limits,
                                       attributes,
                                       shared_context_provider,
                                       type) {}

ContextProviderCommandBuffer::~ContextProviderCommandBuffer() {}

gpu::CommandBufferProxyImpl*
ContextProviderCommandBuffer::GetCommandBufferProxy() {
  return ContextProviderCommandBufferBase::GetCommandBufferProxy();
}
uint32_t ContextProviderCommandBuffer::GetCopyTextureInternalFormat() {
  return ContextProviderCommandBufferBase::GetCopyTextureInternalFormat();
}
void ContextProviderCommandBuffer::SetDefaultTaskRunner(
    scoped_refptr<base::SingleThreadTaskRunner> default_task_runner) {
  return ContextProviderCommandBufferBase::SetDefaultTaskRunner(
      default_task_runner);
}

gpu::ContextResult ContextProviderCommandBuffer::BindToCurrentThread() {
  return ContextProviderCommandBufferBase::BindToCurrentThread();
}
gpu::ContextSupport* ContextProviderCommandBuffer::ContextSupport() {
  return ContextProviderCommandBufferBase::ContextSupport();
}
class GrContext* ContextProviderCommandBuffer::GrContext() {
  return ContextProviderCommandBufferBase::GrContext();
}
viz::ContextCacheController* ContextProviderCommandBuffer::CacheController() {
  return ContextProviderCommandBufferBase::CacheController();
}
void ContextProviderCommandBuffer::InvalidateGrContext(uint32_t state) {
  return ContextProviderCommandBufferBase::InvalidateGrContext(state);
}
base::Lock* ContextProviderCommandBuffer::GetLock() {
  return ContextProviderCommandBufferBase::GetLock();
}
const gpu::Capabilities& ContextProviderCommandBuffer::ContextCapabilities()
    const {
  return ContextProviderCommandBufferBase::ContextCapabilities();
}
const gpu::GpuFeatureInfo& ContextProviderCommandBuffer::GetGpuFeatureInfo()
    const {
  return ContextProviderCommandBufferBase::GetGpuFeatureInfo();
}
void ContextProviderCommandBuffer::AddObserver(viz::ContextLostObserver* obs) {
  return ContextProviderCommandBufferBase::AddObserver(obs);
}
void ContextProviderCommandBuffer::RemoveObserver(
    viz::ContextLostObserver* obs) {
  return ContextProviderCommandBufferBase::RemoveObserver(obs);
}

gpu::gles2::GLES2Interface* ContextProviderCommandBuffer::ContextGL() {
  return ContextProviderCommandBufferBase::ContextGL();
}

///////////////////

RasterContextProviderCommandBuffer::RasterContextProviderCommandBuffer(
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
    command_buffer_metrics::ContextType type)
    : ContextProviderCommandBufferBase(channel,
                                       stream_id,
                                       stream_priority,
                                       surface_handle,
                                       active_url,
                                       automatic_flushes,
                                       support_locking,
                                       memory_limits,
                                       attributes,
                                       shared_context_provider,
                                       type) {}

RasterContextProviderCommandBuffer::~RasterContextProviderCommandBuffer() {}

gpu::CommandBufferProxyImpl*
RasterContextProviderCommandBuffer::GetCommandBufferProxy() {
  return ContextProviderCommandBufferBase::GetCommandBufferProxy();
}
uint32_t RasterContextProviderCommandBuffer::GetCopyTextureInternalFormat() {
  return ContextProviderCommandBufferBase::GetCopyTextureInternalFormat();
}
void RasterContextProviderCommandBuffer::SetDefaultTaskRunner(
    scoped_refptr<base::SingleThreadTaskRunner> default_task_runner) {
  return ContextProviderCommandBufferBase::SetDefaultTaskRunner(
      default_task_runner);
}

gpu::ContextResult RasterContextProviderCommandBuffer::BindToCurrentThread() {
  return ContextProviderCommandBufferBase::BindToCurrentThread();
}

gpu::ContextSupport* RasterContextProviderCommandBuffer::ContextSupport() {
  return ContextProviderCommandBufferBase::ContextSupport();
}
class GrContext* RasterContextProviderCommandBuffer::GrContext() {
  return ContextProviderCommandBufferBase::GrContext();
}
viz::ContextCacheController*
RasterContextProviderCommandBuffer::CacheController() {
  return ContextProviderCommandBufferBase::CacheController();
}
void RasterContextProviderCommandBuffer::InvalidateGrContext(uint32_t state) {
  return ContextProviderCommandBufferBase::InvalidateGrContext(state);
}
base::Lock* RasterContextProviderCommandBuffer::GetLock() {
  return ContextProviderCommandBufferBase::GetLock();
}
const gpu::Capabilities&
RasterContextProviderCommandBuffer::ContextCapabilities() const {
  return ContextProviderCommandBufferBase::ContextCapabilities();
}
const gpu::GpuFeatureInfo&
RasterContextProviderCommandBuffer::GetGpuFeatureInfo() const {
  return ContextProviderCommandBufferBase::GetGpuFeatureInfo();
}
void RasterContextProviderCommandBuffer::AddObserver(
    viz::ContextLostObserver* obs) {
  return ContextProviderCommandBufferBase::AddObserver(obs);
}
void RasterContextProviderCommandBuffer::RemoveObserver(
    viz::ContextLostObserver* obs) {
  return ContextProviderCommandBufferBase::RemoveObserver(obs);
}

gpu::raster::RasterInterface*
RasterContextProviderCommandBuffer::RasterInterface() {
  return ContextProviderCommandBufferBase::RasterInterface();
}

}  // namespace ui
