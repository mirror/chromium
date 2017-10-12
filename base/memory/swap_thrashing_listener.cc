// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/swap_thrashing_listener.h"

#include "base/observer_list_threadsafe.h"
#include "base/trace_event/trace_event.h"

namespace base {

namespace {

class SwapThrashingObserver {
 public:
  SwapThrashingObserver()
      : async_observers_(new ObserverListThreadSafe<SwapThrashingListener>),
        sync_observers_(new ObserverList<SwapThrashingListener>) {}

  void AddObserver(SwapThrashingListener* listener, bool sync) {
    async_observers_->AddObserver(listener);
    if (sync) {
      AutoLock lock(sync_observers_lock_);
      sync_observers_->AddObserver(listener);
    }
  }

  void RemoveObserver(SwapThrashingListener* listener) {
    async_observers_->RemoveObserver(listener);
    AutoLock lock(sync_observers_lock_);
    sync_observers_->RemoveObserver(listener);
  }

  void Notify(SwapThrashingListener::SwapThrashingLevel swap_thrashing_level) {
    async_observers_->Notify(FROM_HERE, &SwapThrashingListener::Notify,
                             swap_thrashing_level);
    AutoLock lock(sync_observers_lock_);
    for (auto& observer : *sync_observers_)
      observer.SwapThrashingListener::SyncNotify(swap_thrashing_level);
  }

 private:
  scoped_refptr<ObserverListThreadSafe<SwapThrashingListener>> async_observers_;
  ObserverList<SwapThrashingListener>* sync_observers_;
  Lock sync_observers_lock_;

  DISALLOW_COPY_AND_ASSIGN(SwapThrashingObserver);
};

SwapThrashingObserver* GetSwapThrashingObserver() {
  static auto* observer = new SwapThrashingObserver();
  return observer;
}

}  // namespace

SwapThrashingListener::SwapThrashingListener(
    const SwapThrashingListener::SwapThrashingCallback& callback)
    : callback_(callback) {
  GetSwapThrashingObserver()->AddObserver(this, false);
}

SwapThrashingListener::SwapThrashingListener(
    const SwapThrashingListener::SwapThrashingCallback& callback,
    const SwapThrashingListener::SyncSwapThrashingCallback&
        sync_swap_thrashing_callback)
    : callback_(callback),
      sync_swap_thrashing_callback_(sync_swap_thrashing_callback) {
  GetSwapThrashingObserver()->AddObserver(this, true);
}

SwapThrashingListener::~SwapThrashingListener() {
  GetSwapThrashingObserver()->RemoveObserver(this);
}

void SwapThrashingListener::Notify(SwapThrashingLevel swap_thrashing_level) {
  callback_.Run(swap_thrashing_level);
}

void SwapThrashingListener::SyncNotify(
    SwapThrashingLevel swap_thrashing_level) {
  if (!sync_swap_thrashing_callback_.is_null()) {
    sync_swap_thrashing_callback_.Run(swap_thrashing_level);
  }
}

// static
void SwapThrashingListener::NotifySwapThrashing(
    SwapThrashingLevel swap_thrashing_level) {
  DCHECK_NE(swap_thrashing_level, SWAP_THRASHING_LEVEL_NONE);
  DoNotifySwapThrashing(swap_thrashing_level);
}

// static
void SwapThrashingListener::DoNotifySwapThrashing(
    SwapThrashingLevel swap_thrashing_level) {
  DCHECK_NE(swap_thrashing_level, SWAP_THRASHING_LEVEL_NONE);

  GetSwapThrashingObserver()->Notify(swap_thrashing_level);
}

}  // namespace base
