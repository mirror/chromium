// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/gpu/context_provider.h"

namespace viz {

ContextProvider::ScopedContextLock::ScopedContextLock(
    ContextProvider* context_provider)
    : context_provider_(context_provider) {
  if (context_provider->GetLock())
    context_lock_.emplace(*context_provider->GetLock());
  context_provider->CheckValidThreadOrLockAcquired();
  busy_ = context_provider_->CacheController()->ClientBecameBusy();
}

ContextProvider::ScopedContextLock::~ScopedContextLock() {
  // Let ContextCacheController know we are no longer busy.
  context_provider_->CacheController()->ClientBecameNotBusy(std::move(busy_));
}

ContextProvider::ContextProvider(bool support_locking)
    : support_locking_(support_locking) {}

base::Lock* ContextProvider::GetLock() {
  if (!support_locking_)
    return nullptr;
  return &context_lock_;
}

}  // namespace viz
