// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/scoped_may_block.h"

#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"

namespace base {

namespace internal {

namespace {

LazyInstance<ThreadLocalPointer<BlockingObserver>>::Leaky
    tls_blocking_observer = LAZY_INSTANCE_INITIALIZER;

BlockingObserver* GetBlockingObserverForCurrentThread() {
  return tls_blocking_observer.Get().Get();
}

}  // namespace

void SetBlockingObserverForCurrentThread(BlockingObserver* blocking_observer) {
  DCHECK(!GetBlockingObserverForCurrentThread());
  tls_blocking_observer.Get().Set(blocking_observer);
}

}  // namespace internal

ScopedMayBlock::ScopedMayBlock() {
  if (internal::GetBlockingObserverForCurrentThread())
    internal::GetBlockingObserverForCurrentThread()->ThreadBlocked();
}

ScopedMayBlock::~ScopedMayBlock() {
  if (internal::GetBlockingObserverForCurrentThread())
    internal::GetBlockingObserverForCurrentThread()->ThreadUnblocked();
}

}  // namespace base
