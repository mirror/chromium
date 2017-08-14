// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/scoped_will_block.h"

#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"

namespace base {

namespace {

LazyInstance<ThreadLocalPointer<internal::BlockingObserver>>::Leaky
    tls_blocking_observer = LAZY_INSTANCE_INITIALIZER;

#if DCHECK_IS_ON()
// Ensures the absence of nested ScopedWillBlock instances.
LazyInstance<ThreadLocalBoolean>::Leaky tls_in_blocked_scope =
    LAZY_INSTANCE_INITIALIZER;
#endif
}  // namespace

namespace internal {

void SetBlockingObserverForCurrentThread(BlockingObserver* blocking_observer) {
  DCHECK(!tls_blocking_observer.Get().Get());
  tls_blocking_observer.Get().Set(blocking_observer);
}

}  // namespace internal

ScopedWillBlock::ScopedWillBlock() {
#if DCHECK_IS_ON()
  DCHECK(!tls_in_blocked_scope.Get().Get());
  tls_in_blocked_scope.Get().Set(true);
#endif

  internal::BlockingObserver* blocking_observer =
      tls_blocking_observer.Get().Get();
  if (blocking_observer)
    blocking_observer->BlockingScopeEntered();
}

ScopedWillBlock::~ScopedWillBlock() {
#if DCHECK_IS_ON()
  DCHECK(tls_in_blocked_scope.Get().Get());
  tls_in_blocked_scope.Get().Set(false);
#endif

  internal::BlockingObserver* blocking_observer =
      tls_blocking_observer.Get().Get();
  if (blocking_observer)
    blocking_observer->BlockingScopeExited();
}

}  // namespace base
