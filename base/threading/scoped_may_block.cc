// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/scoped_may_block.h"

#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"
#include "base/threading/thread_restrictions.h"

namespace base {

namespace {

LazyInstance<ThreadLocalPointer<internal::BlockingObserver>>::Leaky
    tls_blocking_observer = LAZY_INSTANCE_INITIALIZER;

}  // namespace

namespace internal {

void SetBlockingObserverForCurrentThread(BlockingObserver* blocking_observer) {
  DCHECK(!tls_blocking_observer.Get().Get());
  tls_blocking_observer.Get().Set(blocking_observer);
}

}  // namespace internal

ScopedWillBlock::ScopedWillBlock() {
  internal::BlockingObserver* blocking_observer =
      tls_blocking_observer.Get().Get();
  if (blocking_observer) {
    ThreadRestrictions::AssertBlockingAllowed();
#if DCHECK_IS_ON()
    DCHECK(!blocking_observer->in_blocked_scope_);
    blocking_observer->in_blocked_scope_ = true;
#endif
    blocking_observer->BlockingScopeEntered();
  }
}

ScopedWillBlock::~ScopedWillBlock() {
  internal::BlockingObserver* blocking_observer =
      tls_blocking_observer.Get().Get();
  if (blocking_observer) {
#if DCHECK_IS_ON()
    DCHECK(blocking_observer->in_blocked_scope_);
    blocking_observer->in_blocked_scope_ = false;
#endif
    blocking_observer->BlockingScopeExited();
  }
}

}  // namespace base
