// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_COMPOSITOR_TEST_IN_PROCESS_CONTEXT_PROVIDER_H_
#define UI_COMPOSITOR_TEST_IN_PROCESS_CONTEXT_PROVIDER_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_checker.h"
#include "components/viz/common/gpu/context_provider.h"
#include "gpu/command_buffer/common/gles2_cmd_utils.h"
#include "gpu/ipc/common/surface_handle.h"
#include "ui/gfx/native_widget_types.h"

namespace gpu {
class GLInProcessContext;
class GpuMemoryBufferManager;
class ImageFactory;
}

namespace skia_bindings {
class GrContextForGLES2Interface;
}

namespace ui {

class InProcessContextProviderBase {
 public:
  // cc::GLContextProvider implementation.
  gpu::ContextResult BindToCurrentThread();
  const gpu::Capabilities& ContextCapabilities() const;
  const gpu::GpuFeatureInfo& GetGpuFeatureInfo() const;
  gpu::gles2::GLES2Interface* ContextGL();
  gpu::raster::RasterInterface* RasterInterface();
  gpu::ContextSupport* ContextSupport();
  class GrContext* GrContext();
  viz::ContextCacheController* CacheController();
  void InvalidateGrContext(uint32_t state);
  base::Lock* GetLock();
  void AddObserver(viz::ContextLostObserver* obs);
  void RemoveObserver(viz::ContextLostObserver* obs);

  // Gives the GL internal format that should be used for calling CopyTexImage2D
  // on the default framebuffer.
  uint32_t GetCopyTextureInternalFormat();

 protected:
  InProcessContextProviderBase(
      const gpu::gles2::ContextCreationAttribHelper& attribs,
      InProcessContextProviderBase* shared_context,
      gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
      gpu::ImageFactory* image_factory,
      gpu::SurfaceHandle window,
      const std::string& debug_name,
      bool support_locking);
  virtual ~InProcessContextProviderBase();

 private:
  void CheckValidThreadOrLockAcquired() const {
#if DCHECK_IS_ON()
    if (support_locking_) {
      context_lock_.AssertAcquired();
    } else {
      DCHECK(context_thread_checker_.CalledOnValidThread());
    }
#endif
  }

  base::ThreadChecker main_thread_checker_;
  base::ThreadChecker context_thread_checker_;

  std::unique_ptr<gpu::GLInProcessContext> context_;
  std::unique_ptr<skia_bindings::GrContextForGLES2Interface> gr_context_;
  std::unique_ptr<gpu::raster::RasterInterface> raster_context_;
  std::unique_ptr<viz::ContextCacheController> cache_controller_;

  const bool support_locking_ ALLOW_UNUSED_TYPE;
  bool bind_tried_ = false;
  gpu::ContextResult bind_result_;

  gpu::gles2::ContextCreationAttribHelper attribs_;
  InProcessContextProviderBase* shared_context_;
  gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager_;
  gpu::ImageFactory* image_factory_;
  gpu::SurfaceHandle window_;
  std::string debug_name_;

  base::Lock context_lock_;

  DISALLOW_COPY_AND_ASSIGN(InProcessContextProviderBase);
};

class InProcessContextProvider : public viz::GLContextProvider,
                                 public InProcessContextProviderBase {
 public:
  static scoped_refptr<InProcessContextProvider> Create(
      const gpu::gles2::ContextCreationAttribHelper& attribs,
      InProcessContextProviderBase* shared_context,
      gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
      gpu::ImageFactory* image_factory,
      gpu::SurfaceHandle window,
      const std::string& debug_name,
      bool support_locking);

  // Uses default attributes for creating an offscreen context.
  static scoped_refptr<InProcessContextProvider> CreateOffscreen(
      gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
      gpu::ImageFactory* image_factory,
      InProcessContextProviderBase* shared_context,
      bool support_locking);

  // viz::CommonContextProvider implementation.
  gpu::ContextResult BindToCurrentThread() override;
  const gpu::Capabilities& ContextCapabilities() const override;
  const gpu::GpuFeatureInfo& GetGpuFeatureInfo() const override;
  gpu::ContextSupport* ContextSupport() override;
  class GrContext* GrContext() override;
  viz::ContextCacheController* CacheController() override;
  void InvalidateGrContext(uint32_t state) override;
  base::Lock* GetLock() override;
  void AddObserver(viz::ContextLostObserver* obs) override;
  void RemoveObserver(viz::ContextLostObserver* obs) override;

  // viz::GLContextProvider implementation.
  gpu::gles2::GLES2Interface* ContextGL() override;

 protected:
  InProcessContextProvider(
      const gpu::gles2::ContextCreationAttribHelper& attribs,
      InProcessContextProviderBase* shared_context,
      gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
      gpu::ImageFactory* image_factory,
      gpu::SurfaceHandle window,
      const std::string& debug_name,
      bool support_locking);
  ~InProcessContextProvider() override{};
};

class InProcessRasterContextProvider : public viz::RasterContextProvider,
                                       public InProcessContextProviderBase {
 public:
  // Uses default attributes for creating an offscreen context.
  static scoped_refptr<InProcessRasterContextProvider> CreateOffscreen(
      gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
      gpu::ImageFactory* image_factory,
      InProcessContextProviderBase* shared_context,
      bool support_locking);

  // viz::CommonContextProvider implementation.
  gpu::ContextResult BindToCurrentThread() override;
  const gpu::Capabilities& ContextCapabilities() const override;
  const gpu::GpuFeatureInfo& GetGpuFeatureInfo() const override;
  gpu::ContextSupport* ContextSupport() override;
  class GrContext* GrContext() override;
  viz::ContextCacheController* CacheController() override;
  void InvalidateGrContext(uint32_t state) override;
  base::Lock* GetLock() override;
  void AddObserver(viz::ContextLostObserver* obs) override;
  void RemoveObserver(viz::ContextLostObserver* obs) override;

  // viz::RasterContextProvider implementation.
  gpu::raster::RasterInterface* RasterInterface() override;

 protected:
  InProcessRasterContextProvider(
      const gpu::gles2::ContextCreationAttribHelper& attribs,
      InProcessContextProviderBase* shared_context,
      gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
      gpu::ImageFactory* image_factory,
      gpu::SurfaceHandle window,
      const std::string& debug_name,
      bool support_locking);
  ~InProcessRasterContextProvider() override{};
};

}  // namespace ui

#endif  // UI_COMPOSITOR_TEST_IN_PROCESS_CONTEXT_PROVIDER_H_
