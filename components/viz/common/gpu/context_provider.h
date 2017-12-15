// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_GPU_CONTEXT_PROVIDER_H_
#define COMPONENTS_VIZ_COMMON_GPU_CONTEXT_PROVIDER_H_

#include <stddef.h>
#include <stdint.h>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "components/viz/common/gpu/context_cache_controller.h"
#include "components/viz/common/gpu/context_lost_observer.h"
#include "components/viz/common/viz_common_export.h"
#include "gpu/command_buffer/common/capabilities.h"
#include "gpu/command_buffer/common/context_result.h"

class GrContext;

namespace base {
class Lock;
}

namespace gpu {
class ContextSupport;
struct GpuFeatureInfo;

namespace gles2 {
class GLES2Interface;
}
namespace raster {
class RasterInterface;
}
}  // namespace gpu

namespace viz {

class VIZ_COMMON_EXPORT CommonContextInterfaceProvider {
 public:
  // RefCounted interface.
  virtual void AddRef() const = 0;
  virtual void Release() const = 0;

  // Bind the 3d context to the current thread. This should be called before
  // accessing the contexts. Calling it more than once should have no effect.
  // Once this function has been called, the class should only be accessed
  // from the same thread unless the function has some explicitly specified
  // rules for access on a different thread. See SetupLockOnMainThread(), which
  // can be used to provide access from multiple threads.
  virtual gpu::ContextResult BindToCurrentThread() = 0;

  // Adds/removes an observer to be called when the context is lost. AddObserver
  // should be called before BindToCurrentThread from the same thread that the
  // context is bound to, or any time while the lock is acquired after checking
  // for context loss.
  // NOTE: Implementations must avoid post-tasking the to the observer directly
  // as the observer may remove itself before the task runs.
  virtual void AddObserver(ContextLostObserver* obs) = 0;
  virtual void RemoveObserver(ContextLostObserver* obs) = 0;

  // Returns the lock that should be held if using this context from multiple
  // threads. This can be called on any thread.
  // NOTE: Helper method for ScopedContextLock. Use that instead of calling this
  // directly.
  virtual base::Lock* GetLock() = 0;

  // Get a CacheController interface to the 3d context.  Returns nullptr if the
  // context provider was not bound to a thread.
  virtual ContextCacheController* CacheController() = 0;

  // Get a ContextSupport interface to the 3d context.  Returns nullptr if the
  // context provider was not bound to a thread.
  virtual gpu::ContextSupport* ContextSupport() = 0;

  // Get a Raster interface to the 3d context.  Returns nullptr if the context
  // provider was not bound to a thread, or if a GrContext fails to initialize
  // on this context.
  virtual class GrContext* GrContext() = 0;

  // Invalidates the cached OpenGL state in GrContext.
  // See skia GrContext::resetContext for details.
  virtual void InvalidateGrContext(uint32_t state) = 0;

  // Returns the capabilities of the currently bound 3d context.
  virtual const gpu::Capabilities& ContextCapabilities() const = 0;

  // Returns feature blacklist decisions and driver bug workarounds info.
  virtual const gpu::GpuFeatureInfo& GetGpuFeatureInfo() const = 0;
};

class VIZ_COMMON_EXPORT GLContextProvider
    : public virtual CommonContextInterfaceProvider {
 public:
  class VIZ_COMMON_EXPORT ScopedContextLockGL {
   public:
    explicit ScopedContextLockGL(GLContextProvider* context_provider);
    ~ScopedContextLockGL();

    gpu::gles2::GLES2Interface* ContextGL() {
      return context_provider_->ContextGL();
    }

   private:
    GLContextProvider* const context_provider_;
    base::AutoLock context_lock_;
    std::unique_ptr<ContextCacheController::ScopedBusy> busy_;
  };

  // Get a GLES2 interface to the 3d context.  Returns nullptr if the context
  // provider was not bound to a thread, or if the GLES2 interface is not
  // supported by this context.
  virtual gpu::gles2::GLES2Interface* ContextGL() = 0;
};

class VIZ_COMMON_EXPORT RasterContextProvider
    : public virtual CommonContextInterfaceProvider {
 public:
  class VIZ_COMMON_EXPORT ScopedContextLockRaster {
   public:
    explicit ScopedContextLockRaster(RasterContextProvider* context_provider);
    ~ScopedContextLockRaster();

    gpu::raster::RasterInterface* RasterInterface() {
      return context_provider_->RasterInterface();
    }

   private:
    RasterContextProvider* const context_provider_;
    base::AutoLock context_lock_;
    std::unique_ptr<ContextCacheController::ScopedBusy> busy_;
  };

  // Get a Raster interface to the 3d context.  Returns nullptr if the context
  // provider was not bound to a thread, or if the Raster interface is not
  // supported by this context.
  virtual gpu::raster::RasterInterface* RasterInterface() = 0;
};

class VIZ_COMMON_EXPORT ContextProvider
    : public base::RefCountedThreadSafe<ContextProvider>,
      public GLContextProvider,
      public RasterContextProvider {
 public:
  class VIZ_COMMON_EXPORT ScopedContextLock {
   public:
    explicit ScopedContextLock(ContextProvider* context_provider);
    ~ScopedContextLock();

   private:
    ContextProvider* const context_provider_;
    base::AutoLock context_lock_;
    std::unique_ptr<ContextCacheController::ScopedBusy> busy_;
  };

  void AddRef() const override;
  void Release() const override;

 protected:
  friend class base::RefCountedThreadSafe<ContextProvider>;

  virtual ~ContextProvider() {}
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_GPU_CONTEXT_PROVIDER_H_
