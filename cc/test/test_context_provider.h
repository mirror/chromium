// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_TEST_CONTEXT_PROVIDER_H_
#define CC_TEST_TEST_CONTEXT_PROVIDER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_checker.h"
#include "cc/test/test_context_support.h"
#include "components/viz/common/gpu/context_provider.h"
#include "gpu/command_buffer/client/gles2_interface_stub.h"
#include "gpu/config/gpu_feature_info.h"
#include "third_party/skia/include/core/SkRefCnt.h"

namespace skia_bindings {
class GrContextForGLES2Interface;
}

namespace cc {
class TestWebGraphicsContext3D;
class TestGLES2Interface;

class TestContextProviderBase {
 public:
  typedef base::Callback<std::unique_ptr<TestWebGraphicsContext3D>(void)>
      CreateCallback;

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

  TestWebGraphicsContext3D* TestContext3d();

  // This returns the TestWebGraphicsContext3D but is valid to call
  // before the context is bound to a thread. This is needed to set up
  // state on the test context before binding. Don't call
  // InitializeOnCurrentThread on the context returned from this method.
  TestWebGraphicsContext3D* UnboundTestContext3d();

  TestGLES2Interface* TestContextGL() { return context_gl_.get(); }
  TestContextSupport* support() { return support_.get(); }

 protected:
  explicit TestContextProviderBase(
      std::unique_ptr<TestContextSupport> support,
      std::unique_ptr<TestGLES2Interface> gl,
      std::unique_ptr<TestWebGraphicsContext3D> context,
      bool support_locking);
  virtual ~TestContextProviderBase();

 private:
  void OnLostContext();
  void CheckValidThreadOrLockAcquired() const {
#if DCHECK_IS_ON()
    if (support_locking_) {
      context_lock_.AssertAcquired();
    } else {
      DCHECK(context_thread_checker_.CalledOnValidThread());
    }
#endif
  }

  std::unique_ptr<TestContextSupport> support_;
  std::unique_ptr<TestWebGraphicsContext3D> context3d_;
  std::unique_ptr<TestGLES2Interface> context_gl_;
  std::unique_ptr<gpu::raster::RasterInterface> raster_context_;
  std::unique_ptr<skia_bindings::GrContextForGLES2Interface> gr_context_;
  std::unique_ptr<viz::ContextCacheController> cache_controller_;
  const bool support_locking_ ALLOW_UNUSED_TYPE;
  bool bound_ = false;

  gpu::GpuFeatureInfo gpu_feature_info_;

  base::ThreadChecker main_thread_checker_;
  base::ThreadChecker context_thread_checker_;

  base::Lock context_lock_;

  base::ObserverList<viz::ContextLostObserver> observers_;

  base::WeakPtrFactory<TestContextProviderBase> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TestContextProviderBase);
};

class TestContextProvider : public viz::GLContextProvider,
                            public TestContextProviderBase {
 public:
  typedef base::Callback<std::unique_ptr<TestWebGraphicsContext3D>(void)>
      CreateCallback;

  static scoped_refptr<TestContextProvider> Create();
  static scoped_refptr<TestContextProvider> Create(
      std::unique_ptr<TestWebGraphicsContext3D> context);
  static scoped_refptr<TestContextProvider> Create(
      std::unique_ptr<TestWebGraphicsContext3D> context,
      std::unique_ptr<TestContextSupport> support);
  static scoped_refptr<TestContextProvider> Create(
      std::unique_ptr<TestGLES2Interface> gl);

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
  explicit TestContextProvider(
      std::unique_ptr<TestContextSupport> support,
      std::unique_ptr<TestGLES2Interface> gl,
      std::unique_ptr<TestWebGraphicsContext3D> context,
      bool support_locking);
  ~TestContextProvider() override{};
};

class TestRasterContextProvider : public viz::RasterContextProvider,
                                  public TestContextProviderBase {
 public:
  typedef base::Callback<std::unique_ptr<TestWebGraphicsContext3D>(void)>
      CreateCallback;

  static scoped_refptr<TestRasterContextProvider> CreateWorker();
  static scoped_refptr<TestRasterContextProvider> CreateWorker(
      std::unique_ptr<TestWebGraphicsContext3D> context,
      std::unique_ptr<TestContextSupport> support);

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
  explicit TestRasterContextProvider(
      std::unique_ptr<TestContextSupport> support,
      std::unique_ptr<TestGLES2Interface> gl,
      std::unique_ptr<TestWebGraphicsContext3D> context,
      bool support_locking);
  ~TestRasterContextProvider() override{};
};

}  // namespace cc

#endif  // CC_TEST_TEST_CONTEXT_PROVIDER_H_
