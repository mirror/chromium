// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/allocator/partition_allocator/event_lock.h"

#include "base/allocator/partition_allocator/lock_util.h"
#include "build/build_config.h"

namespace {

bool IsEvent(intptr_t current) {
  return current != base::subtle::EventLock::UNLOCKED &&
         current != base::subtle::EventLock::LOCKED &&
         current != base::subtle::EventLock::CONTENDED;
}

}  // anonymous namespace

namespace base {
namespace subtle {

EventLock::EventLock() {
  lock_.store(UNLOCKED, std::memory_order_release);
}

EventLock::~EventLock() {
  intptr_t current = lock_.load(std::memory_order_acquire);
  if (UNLIKELY(IsEvent(current))) {
    WaitableEvent* event = reinterpret_cast<WaitableEvent*>(current);
    delete event;
  }
}

void EventLock::LockSlow(intptr_t current) {
  // The value of |kYieldProcessorTries| is cargo culted from TCMalloc, Windows
  // critical section defaults, and various other recommendations. TODO(jschuh):
  // Further tuning may be warranted.
  static const int kYieldProcessorTries = 1000;

  // Spin until the lock becomes an event:
  do {
    for (int count = 0; count < kYieldProcessorTries; ++count) {
      // Let the processor know we're spinning.
      YIELD_PROCESSOR;
      current = lock_.load(std::memory_order_acquire);
      DCHECK_NE(UNLOCKED, current);
      if (LOCKED == current) {
        lock_.exchange(CONTENDED, std::memory_order_acquire);
      }
    }

    // Give the OS a chance to schedule something on this core.
    YIELD_THREAD;
  } while (!IsEvent(current));

  DCHECK(IsEvent(current));
  base::WaitableEvent* event = reinterpret_cast<base::WaitableEvent*>(current);
  event->Wait();
}

void EventLock::UnlockSlow(intptr_t current) {
  WaitableEvent* event;
  if (IsEvent(current)) {
    event = reinterpret_cast<WaitableEvent*>(current);
  } else {
    event = new WaitableEvent(WaitableEvent::ResetPolicy::AUTOMATIC,
                              WaitableEvent::InitialState::NOT_SIGNALED);
    lock_.store(reinterpret_cast<intptr_t>(event), std::memory_order_release);
  }

  event->Signal();
}

}  // namespace subtle
}  // namespace base
