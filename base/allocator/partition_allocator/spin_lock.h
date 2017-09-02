// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ALLOCATOR_PARTITION_ALLOCATOR_SPIN_LOCK_H
#define BASE_ALLOCATOR_PARTITION_ALLOCATOR_SPIN_LOCK_H

#include <atomic>
#include <memory>
#include <mutex>

#include "base/base_export.h"
#include "base/compiler_specific.h"
#include "base/synchronization/waitable_event.h"

// Spinlock is a simple spinlock class based on the standard CPU primitive of
// atomic increment and decrement of an int at a given memory address. These are
// intended only for very short duration locks and assume a system with multiple
// cores. For any potentially longer wait you should use a real lock, such as
// |base::Lock|.
//
// |SpinLock|s MUST be globals. Using them as (e.g.) struct/class members will
// result in an uninitialized lock, which is dangerously incorrect.

namespace base {
namespace subtle {

class SpinLock {
 public:
  using Guard = std::lock_guard<SpinLock>;

  enum LockState { UNLOCKED = 0, LOCKED = -1, CONTENDED = -2 };

  BASE_EXPORT SpinLock();

  BASE_EXPORT ~SpinLock();

  ALWAYS_INLINE void lock() {
    static_assert(sizeof(lock_) == sizeof(intptr_t),
                  "intptr_t and lock_ are different sizes");
    intptr_t current = lock_.load(std::memory_order_acquire);
    if (LIKELY(current == UNLOCKED)) {
      lock_.store(LOCKED, std::memory_order_release);
      return;
    }
    LockSlow(current);
  }

  ALWAYS_INLINE void unlock() {
    intptr_t current = lock_.load(std::memory_order_acquire);
    DCHECK_NE(UNLOCKED, current);
    if (LIKELY(current == LOCKED)) {
      lock_.store(UNLOCKED, std::memory_order_release);
      return;
    }
    UnlockSlow(current);
  }

 private:
  // |lock| and |unlock| need to be small, so that they can also be inlined for
  // maximum speed. If we have to fall back to using |base::WaitableEvent|, they
  // call these utility routines.
  BASE_EXPORT void LockSlow(intptr_t current);
  BASE_EXPORT void UnlockSlow(intptr_t current);

  std::atomic_intptr_t lock_;
};

}  // namespace subtle
}  // namespace base

#endif  // BASE_ALLOCATOR_PARTITION_ALLOCATOR_SPIN_LOCK_H
