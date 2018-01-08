// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/scoped_blocking_call.h"

#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"

namespace base {

namespace {

ThreadLocalPointer<internal::BlockingObserver>* tls_blocking_observer = nullptr;

// Last ScopedBlockingCall instantiated on this thread.
ThreadLocalPointer<ScopedBlockingCall>* tls_last_scoped_blocking_call = nullptr;

bool ScopedBlockingCallIsInitialized() {
  DCHECK_EQ(!!tls_blocking_observer, !!tls_last_scoped_blocking_call);
  return tls_blocking_observer;
}

}  // namespace

ScopedBlockingCall::ScopedBlockingCall(BlockingType blocking_type)
    : blocking_observer_(ScopedBlockingCallIsInitialized()
                             ? tls_blocking_observer->Get()
                             : nullptr),
      previous_scoped_blocking_call_(ScopedBlockingCallIsInitialized()
                                         ? tls_last_scoped_blocking_call->Get()
                                         : nullptr),
      is_will_block_(blocking_type == BlockingType::WILL_BLOCK ||
                     (previous_scoped_blocking_call_ &&
                      previous_scoped_blocking_call_->is_will_block_)) {
  if (!ScopedBlockingCallIsInitialized())
    return;

  tls_last_scoped_blocking_call->Set(this);

  if (blocking_observer_) {
    if (!previous_scoped_blocking_call_) {
      blocking_observer_->BlockingStarted(blocking_type);
    } else if (blocking_type == BlockingType::WILL_BLOCK &&
               !previous_scoped_blocking_call_->is_will_block_) {
      blocking_observer_->BlockingTypeUpgraded();
    }
  }
}

ScopedBlockingCall::~ScopedBlockingCall() {
  if (!ScopedBlockingCallIsInitialized())
    return;

  DCHECK_EQ(this, tls_last_scoped_blocking_call->Get());
  tls_last_scoped_blocking_call->Set(previous_scoped_blocking_call_);
  if (blocking_observer_ && !previous_scoped_blocking_call_)
    blocking_observer_->BlockingEnded();
}

namespace internal {

void InitializeScopedBlockingCall() {
  if (ScopedBlockingCallIsInitialized())
    return;
  tls_blocking_observer = new ThreadLocalPointer<internal::BlockingObserver>();
  tls_last_scoped_blocking_call = new ThreadLocalPointer<ScopedBlockingCall>();
}

void SetBlockingObserverForCurrentThread(BlockingObserver* blocking_observer) {
  DCHECK(ScopedBlockingCallIsInitialized());
  tls_blocking_observer->Set(blocking_observer);
}

void ClearBlockingObserverForTesting() {
  DCHECK(ScopedBlockingCallIsInitialized());
  tls_blocking_observer->Set(nullptr);
}

ScopedClearBlockingObserverForTesting::ScopedClearBlockingObserverForTesting()
    : blocking_observer_(tls_blocking_observer->Get()) {
  ClearBlockingObserverForTesting();
}

ScopedClearBlockingObserverForTesting::
    ~ScopedClearBlockingObserverForTesting() {
  DCHECK(ScopedBlockingCallIsInitialized());
  DCHECK(!tls_blocking_observer->Get());
  SetBlockingObserverForCurrentThread(blocking_observer_);
}

}  // namespace internal

}  // namespace base
