// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/scoped_blocking_call.h"

#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"

namespace base {

namespace {

LazyInstance<ThreadLocalPointer<internal::BlockingObserver>>::Leaky
    tls_blocking_observer = LAZY_INSTANCE_INITIALIZER;

// Last ScopedBlockingCall instantiated on this thread that caused
// BlockingScopeEntered() to be called.
LazyInstance<ThreadLocalPointer<ScopedBlockingCall>>::Leaky
    tls_last_scoped_blocking_call_to_notify_observer =
        LAZY_INSTANCE_INITIALIZER;

#if DCHECK_IS_ON()
// Last ScopedBlockingCall instantiated on this thread.
LazyInstance<ThreadLocalPointer<ScopedBlockingCall>>::Leaky
    tls_last_scoped_blocking_call = LAZY_INSTANCE_INITIALIZER;
#endif

}  // namespace

namespace internal {

void SetBlockingObserverForCurrentThread(BlockingObserver* blocking_observer) {
  DCHECK(!tls_blocking_observer.Get().Get());
  tls_blocking_observer.Get().Set(blocking_observer);
}

void ClearBlockingObserverForTesting() {
  tls_blocking_observer.Get().Set(nullptr);
}

}  // namespace internal

ScopedBlockingCall::ScopedBlockingCall(BlockingType blocking_type)
    : blocking_type_(blocking_type),
      blocking_observer_(tls_blocking_observer.Get().Get()),
      previous_scoped_blocking_call_to_notify_observer_(
          tls_last_scoped_blocking_call_to_notify_observer.Get().Get())
#if DCHECK_IS_ON()
      ,
      previous_scoped_blocking_call_(tls_last_scoped_blocking_call.Get().Get())
#endif
{
#if DCHECK_IS_ON()
  tls_last_scoped_blocking_call.Get().Set(this);
#endif
  if (blocking_observer_ &&
      ((blocking_type_ == BlockingType::MAY_BLOCK &&
        !previous_scoped_blocking_call_to_notify_observer_) ||
       (blocking_type_ == BlockingType::WILL_BLOCK &&
        (!previous_scoped_blocking_call_to_notify_observer_ ||
         previous_scoped_blocking_call_to_notify_observer_->blocking_type_ ==
             BlockingType::MAY_BLOCK)))) {
    blocking_observer_->BlockingScopeEntered(blocking_type_);
    tls_last_scoped_blocking_call_to_notify_observer.Get().Set(this);
  }
}

ScopedBlockingCall::~ScopedBlockingCall() {
#if DCHECK_IS_ON()
  DCHECK_EQ(blocking_observer_, tls_blocking_observer.Get().Get());
  DCHECK_EQ(this, tls_last_scoped_blocking_call.Get().Get());
  tls_last_scoped_blocking_call.Get().Set(previous_scoped_blocking_call_);
#endif
  if (blocking_observer_ &&
      !previous_scoped_blocking_call_to_notify_observer_) {
    blocking_observer_->BlockingScopeExited();
    tls_last_scoped_blocking_call_to_notify_observer.Get().Set(nullptr);
  }
}

}  // namespace base
