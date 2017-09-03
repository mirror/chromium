// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/allocator/partition_allocator/spin_lock.h"

#include "base/allocator/partition_allocator/lock_util.h"
#include "build/build_config.h"

namespace base {
namespace subtle {

void SpinLock::LockSlow() {
  // The value of |kYieldProcessorTries| is cargo culted from TCMalloc, Windows
  // critical section defaults, and various other recommendations.
  // TODO(jschuh): Further tuning may be warranted.
  static const int kYieldProcessorTries = 1000;
  do {
    do {
      for (int count = 0; count < kYieldProcessorTries; ++count) {
        // Let the processor know we're spinning.
        YIELD_PROCESSOR;
        if (!lock_.load(std::memory_order_relaxed) &&
            LIKELY(!lock_.exchange(true, std::memory_order_acquire)))
          return;
      }

      // Give the OS a chance to schedule something on this core.
      YIELD_THREAD;
    } while (lock_.load(std::memory_order_relaxed));
  } while (UNLIKELY(lock_.exchange(true, std::memory_order_acquire)));
}

}  // namespace subtle
}  // namespace base
