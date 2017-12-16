// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/gpu/context_provider.h"

namespace viz {

CommonContextProvider::CommonContextProvider(scoped_refptr<viz::MultiContextProvider> context_provider)
 : cp_(context_provider) {}

RasterContextProvider::ScopedContextLockRaster::ScopedContextLockRaster(
    RasterContextProvider* context_provider)
    : context_provider_(context_provider),
      context_lock_(*context_provider_->GetLock()) {
  busy_ = context_provider_->CacheController()->ClientBecameBusy();
}

RasterContextProvider::ScopedContextLockRaster::~ScopedContextLockRaster() {
  // Let ContextCacheController know we are no longer busy.
  context_provider_->CacheController()->ClientBecameNotBusy(std::move(busy_));
}

RasterContextProvider::RasterContextProvider(scoped_refptr<viz::MultiContextProvider> context_provider)
 : CommonContextProvider(context_provider) {}
RasterContextProvider::~RasterContextProvider() {}

GLContextProvider::ScopedContextLockGL::ScopedContextLockGL(
    GLContextProvider* context_provider)
    : context_provider_(context_provider),
      context_lock_(*context_provider_->GetLock()) {
  busy_ = context_provider_->CacheController()->ClientBecameBusy();
}

GLContextProvider::ScopedContextLockGL::~ScopedContextLockGL() {
  // Let ContextCacheController know we are no longer busy.
  context_provider_->CacheController()->ClientBecameNotBusy(std::move(busy_));
}

GLContextProvider::GLContextProvider(scoped_refptr<viz::MultiContextProvider> context_provider)
 : CommonContextProvider(context_provider) {}
GLContextProvider::~GLContextProvider() {}

MultiContextProvider::ScopedContextLock::ScopedContextLock(
    MultiContextProvider* context_provider)
    : context_provider_(context_provider),
      context_lock_(*context_provider_->GetLock()) {
  busy_ = context_provider_->CacheController()->ClientBecameBusy();
}

MultiContextProvider::ScopedContextLock::~ScopedContextLock() {
  // Let ContextCacheController know we are no longer busy.
  context_provider_->CacheController()->ClientBecameNotBusy(std::move(busy_));
}

}  // namespace viz
