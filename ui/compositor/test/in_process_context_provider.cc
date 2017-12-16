// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/test/in_process_context_provider.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "components/viz/common/gpu/context_cache_controller.h"
#include "gpu/command_buffer/client/gles2_implementation.h"
#include "gpu/command_buffer/client/gles2_lib.h"
#include "gpu/command_buffer/client/raster_implementation_gles.h"
#include "gpu/command_buffer/client/shared_memory_limits.h"
#include "gpu/ipc/gl_in_process_context.h"
#include "gpu/skia_bindings/grcontext_for_gles2_interface.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "third_party/skia/include/gpu/gl/GrGLInterface.h"

namespace ui {

// static
scoped_refptr<InProcessContextProvider> InProcessContextProvider::Create(
    const gpu::gles2::ContextCreationAttribHelper& attribs,
    InProcessContextProviderBase* shared_context,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    gpu::ImageFactory* image_factory,
    gpu::SurfaceHandle window,
    const std::string& debug_name,
    bool support_locking) {
  return new InProcessContextProvider(attribs, shared_context,
                                      gpu_memory_buffer_manager, image_factory,
                                      window, debug_name, support_locking);
}

// static
scoped_refptr<InProcessContextProvider>
InProcessContextProvider::CreateOffscreen(
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    gpu::ImageFactory* image_factory,
    InProcessContextProviderBase* shared_context,
    bool support_locking) {
  gpu::gles2::ContextCreationAttribHelper attribs;
  attribs.alpha_size = 8;
  attribs.blue_size = 8;
  attribs.green_size = 8;
  attribs.red_size = 8;
  attribs.depth_size = 0;
  attribs.stencil_size = 8;
  attribs.samples = 0;
  attribs.sample_buffers = 0;
  attribs.fail_if_major_perf_caveat = false;
  attribs.bind_generates_resource = false;
  return new InProcessContextProvider(
      attribs, shared_context, gpu_memory_buffer_manager, image_factory,
      gpu::kNullSurfaceHandle, "Offscreen", support_locking);
}

// static
scoped_refptr<InProcessRasterContextProvider>
InProcessRasterContextProvider::CreateOffscreen(
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    gpu::ImageFactory* image_factory,
    InProcessContextProviderBase* shared_context,
    bool support_locking) {
  gpu::gles2::ContextCreationAttribHelper attribs;
  attribs.alpha_size = 8;
  attribs.blue_size = 8;
  attribs.green_size = 8;
  attribs.red_size = 8;
  attribs.depth_size = 0;
  attribs.stencil_size = 8;
  attribs.samples = 0;
  attribs.sample_buffers = 0;
  attribs.fail_if_major_perf_caveat = false;
  attribs.bind_generates_resource = false;
  return new InProcessRasterContextProvider(
      attribs, shared_context, gpu_memory_buffer_manager, image_factory,
      gpu::kNullSurfaceHandle, "Offscreen", support_locking);
}

InProcessContextProviderBase::InProcessContextProviderBase(
    const gpu::gles2::ContextCreationAttribHelper& attribs,
    InProcessContextProviderBase* shared_context,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    gpu::ImageFactory* image_factory,
    gpu::SurfaceHandle window,
    const std::string& debug_name,
    bool support_locking)
    : support_locking_(support_locking),
      attribs_(attribs),
      shared_context_(shared_context),
      gpu_memory_buffer_manager_(gpu_memory_buffer_manager),
      image_factory_(image_factory),
      window_(window),
      debug_name_(debug_name) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  context_thread_checker_.DetachFromThread();
}

InProcessContextProviderBase::~InProcessContextProviderBase() {
  DCHECK(main_thread_checker_.CalledOnValidThread() ||
         context_thread_checker_.CalledOnValidThread());
}

gpu::ContextResult InProcessContextProviderBase::BindToCurrentThread() {
  // This is called on the thread the context will be used.
  DCHECK(context_thread_checker_.CalledOnValidThread());

  if (bind_tried_)
    return bind_result_;
  bind_tried_ = true;

  context_ = gpu::GLInProcessContext::CreateWithoutInit();
  bind_result_ = context_->Initialize(
      nullptr,  /* service */
      nullptr,  /* surface */
      !window_, /* is_offscreen */
      window_, (shared_context_ ? shared_context_->context_.get() : nullptr),
      attribs_, gpu::SharedMemoryLimits(), gpu_memory_buffer_manager_,
      image_factory_, nullptr /* gpu_channel_manager_delegate */,
      base::ThreadTaskRunnerHandle::Get());

  if (bind_result_ != gpu::ContextResult::kSuccess)
    return bind_result_;

  cache_controller_ = std::make_unique<viz::ContextCacheController>(
      context_->GetImplementation(), base::ThreadTaskRunnerHandle::Get());

  std::string unique_context_name =
      base::StringPrintf("%s-%p", debug_name_.c_str(), context_.get());
  context_->GetImplementation()->TraceBeginCHROMIUM(
      "gpu_toplevel", unique_context_name.c_str());

  raster_context_ = std::make_unique<gpu::raster::RasterImplementationGLES>(
      context_->GetImplementation(),
      context_->GetImplementation()->capabilities());

  return bind_result_;
}

const gpu::Capabilities& InProcessContextProviderBase::ContextCapabilities()
    const {
  CheckValidThreadOrLockAcquired();
  return context_->GetImplementation()->capabilities();
}

const gpu::GpuFeatureInfo& InProcessContextProviderBase::GetGpuFeatureInfo()
    const {
  CheckValidThreadOrLockAcquired();
  return context_->GetGpuFeatureInfo();
}

gpu::gles2::GLES2Interface* InProcessContextProviderBase::ContextGL() {
  CheckValidThreadOrLockAcquired();

  return context_->GetImplementation();
}

gpu::raster::RasterInterface* InProcessContextProviderBase::RasterInterface() {
  CheckValidThreadOrLockAcquired();

  return raster_context_.get();
}

gpu::ContextSupport* InProcessContextProviderBase::ContextSupport() {
  CheckValidThreadOrLockAcquired();

  return context_->GetImplementation();
}

class GrContext* InProcessContextProviderBase::GrContext() {
  CheckValidThreadOrLockAcquired();

  if (gr_context_)
    return gr_context_->get();

  size_t max_resource_cache_bytes;
  size_t max_glyph_cache_texture_bytes;
  skia_bindings::GrContextForGLES2Interface::DefaultCacheLimitsForTests(
      &max_resource_cache_bytes, &max_glyph_cache_texture_bytes);
  gr_context_.reset(new skia_bindings::GrContextForGLES2Interface(
      ContextGL(), ContextCapabilities(), max_resource_cache_bytes,
      max_glyph_cache_texture_bytes));
  cache_controller_->SetGrContext(gr_context_->get());

  return gr_context_->get();
}

viz::ContextCacheController* InProcessContextProviderBase::CacheController() {
  CheckValidThreadOrLockAcquired();
  return cache_controller_.get();
}

void InProcessContextProviderBase::InvalidateGrContext(uint32_t state) {
  CheckValidThreadOrLockAcquired();

  if (gr_context_)
    gr_context_->ResetContext(state);
}

base::Lock* InProcessContextProviderBase::GetLock() {
  return &context_lock_;
}

void InProcessContextProviderBase::AddObserver(viz::ContextLostObserver* obs) {
  // Pixel tests do not test lost context.
}

void InProcessContextProviderBase::RemoveObserver(
    viz::ContextLostObserver* obs) {
  // Pixel tests do not test lost context.
}

uint32_t InProcessContextProviderBase::GetCopyTextureInternalFormat() {
  if (attribs_.alpha_size > 0)
    return GL_RGBA;
  DCHECK_NE(attribs_.red_size, 0);
  DCHECK_NE(attribs_.green_size, 0);
  DCHECK_NE(attribs_.blue_size, 0);
  return GL_RGB;
}

InProcessContextProvider::InProcessContextProvider(
    const gpu::gles2::ContextCreationAttribHelper& attribs,
    InProcessContextProviderBase* shared_context,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    gpu::ImageFactory* image_factory,
    gpu::SurfaceHandle window,
    const std::string& debug_name,
    bool support_locking)
    : InProcessContextProviderBase(attribs,
                                   shared_context,
                                   gpu_memory_buffer_manager,
                                   image_factory,
                                   window,
                                   debug_name,
                                   support_locking) {}

gpu::ContextResult InProcessContextProvider::BindToCurrentThread() {
  return InProcessContextProviderBase::BindToCurrentThread();
}
gpu::ContextSupport* InProcessContextProvider::ContextSupport() {
  return InProcessContextProviderBase::ContextSupport();
}
class GrContext* InProcessContextProvider::GrContext() {
  return InProcessContextProviderBase::GrContext();
}
viz::ContextCacheController* InProcessContextProvider::CacheController() {
  return InProcessContextProviderBase::CacheController();
}
void InProcessContextProvider::InvalidateGrContext(uint32_t state) {
  return InProcessContextProviderBase::InvalidateGrContext(state);
}
base::Lock* InProcessContextProvider::GetLock() {
  return InProcessContextProviderBase::GetLock();
}
const gpu::Capabilities& InProcessContextProvider::ContextCapabilities() const {
  return InProcessContextProviderBase::ContextCapabilities();
}
const gpu::GpuFeatureInfo& InProcessContextProvider::GetGpuFeatureInfo() const {
  return InProcessContextProviderBase::GetGpuFeatureInfo();
}
void InProcessContextProvider::AddObserver(viz::ContextLostObserver* obs) {
  return InProcessContextProviderBase::AddObserver(obs);
}
void InProcessContextProvider::RemoveObserver(viz::ContextLostObserver* obs) {
  return InProcessContextProviderBase::RemoveObserver(obs);
}

gpu::gles2::GLES2Interface* InProcessContextProvider::ContextGL() {
  return InProcessContextProviderBase::ContextGL();
}

InProcessRasterContextProvider::InProcessRasterContextProvider(
    const gpu::gles2::ContextCreationAttribHelper& attribs,
    InProcessContextProviderBase* shared_context,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    gpu::ImageFactory* image_factory,
    gpu::SurfaceHandle window,
    const std::string& debug_name,
    bool support_locking)
    : InProcessContextProviderBase(attribs,
                                   shared_context,
                                   gpu_memory_buffer_manager,
                                   image_factory,
                                   window,
                                   debug_name,
                                   support_locking) {}

gpu::ContextResult InProcessRasterContextProvider::BindToCurrentThread() {
  return InProcessContextProviderBase::BindToCurrentThread();
}
gpu::ContextSupport* InProcessRasterContextProvider::ContextSupport() {
  return InProcessContextProviderBase::ContextSupport();
}
class GrContext* InProcessRasterContextProvider::GrContext() {
  return InProcessContextProviderBase::GrContext();
}
viz::ContextCacheController* InProcessRasterContextProvider::CacheController() {
  return InProcessContextProviderBase::CacheController();
}
void InProcessRasterContextProvider::InvalidateGrContext(uint32_t state) {
  return InProcessContextProviderBase::InvalidateGrContext(state);
}
base::Lock* InProcessRasterContextProvider::GetLock() {
  return InProcessContextProviderBase::GetLock();
}
const gpu::Capabilities& InProcessRasterContextProvider::ContextCapabilities()
    const {
  return InProcessContextProviderBase::ContextCapabilities();
}
const gpu::GpuFeatureInfo& InProcessRasterContextProvider::GetGpuFeatureInfo()
    const {
  return InProcessContextProviderBase::GetGpuFeatureInfo();
}
void InProcessRasterContextProvider::AddObserver(
    viz::ContextLostObserver* obs) {
  return InProcessContextProviderBase::AddObserver(obs);
}
void InProcessRasterContextProvider::RemoveObserver(
    viz::ContextLostObserver* obs) {
  return InProcessContextProviderBase::RemoveObserver(obs);
}

gpu::raster::RasterInterface*
InProcessRasterContextProvider::RasterInterface() {
  return InProcessContextProviderBase::RasterInterface();
}

}  // namespace ui
